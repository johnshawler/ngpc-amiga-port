#!/usr/bin/env python3
"""Linux headless libretro oracle. Outputs private JSONL and optional state bytes."""
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
BUTTONS = {'up': 4, 'down': 5, 'left': 6, 'right': 7, 'a': 0, 'b': 8, 'option': 3}

def digest(data):
    return hashlib.sha256(data).hexdigest()

def read_inputs(path):
    events = {} if path is None else json.loads(Path(path).read_text())
    if not isinstance(events, dict):
        raise ValueError('Input must be an object mapping zero-based frames to button lists')
    result = {}
    for frame, buttons in events.items():
        if int(frame) < 0 or str(int(frame)) != frame or not isinstance(buttons, list):
            raise ValueError('Invalid input frame or button list')
        result[int(frame)] = sum(1 << BUTTONS[b] for b in set(buttons))
    return result

class Info(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p), ('extensions', C.c_char_p), ('fullpath', C.c_bool), ('extract', C.c_bool)]
class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]
class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
POLL = C.CFUNCTYPE(None)
INPUT = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path)
    p.add_argument('--core', type=Path, default=ROOT / 'build/mednafen_ngp_libretro.so')
    p.add_argument('--snapshot-dir', type=Path, help='Private per-frame CPU/RAM/GPU captures; requires instrumented core')
    p.add_argument('--scene-check', action='store_true', help='Require native/reference scanline parity from scene-instrumented core')
    p.add_argument('--frames', type=int, default=120)
    p.add_argument('--inputs', type=Path, help='JSON frame -> NGPC button list; absent frames release all buttons')
    p.add_argument('--epoch', type=int, default=946684800)
    p.add_argument('--state-out', type=Path)
    p.add_argument('--replay-check', type=int, default=0, help='Restore final state and compare this many additional frames')
    args = p.parse_args()
    if args.frames < 0 or args.replay_check < 0:
        p.error('Frame counts must be nonnegative')
    # Preloading before Python starts ensures core time() resolves to our clock.
    shim = str(ROOT / 'build/clock.so')
    if os.environ.get('NGPC_ORACLE_CLOCK') != shim:
        os.execve(sys.executable, [sys.executable, str(Path(__file__).resolve()), *sys.argv[1:]],
                  dict(os.environ, LD_PRELOAD=shim, NGPC_ORACLE_CLOCK=shim, TZ='UTC'))
    import time
    time.tzset()
    clock = C.CDLL(shim)
    clock.reference_set_time.argtypes = [C.c_longlong]
    clock.reference_set_time(args.epoch)
    # Python time.time() uses clock_gettime; validate the API used by the core.
    process = C.CDLL(None)
    process.time.argtypes = [C.c_void_p]
    process.time.restype = C.c_long
    if process.time(None) != args.epoch:
        raise RuntimeError('Clock interposition failed')
    core_path = args.core.resolve()
    core = C.CDLL(str(core_path))
    events = read_inputs(args.inputs)
    mask = 0
    pixels = None
    width = height = 0
    audio = bytearray()
    callback_errors = []
    with tempfile.TemporaryDirectory(prefix='ngpc-oracle-') as saves:
        directory = saves.encode()
        @ENV
        def environment(cmd, data):
            if cmd in (9, 31):
                C.cast(data, C.POINTER(C.c_char_p))[0] = directory
                return True
            if cmd == 10:
                return C.cast(data, C.POINTER(C.c_int))[0] == 2  # RGB565
            if cmd == 15:
                var = C.cast(data, C.POINTER(Variable)).contents
                if var.key == b'ngp_language':
                    var.value = b'english'
                    return True
                return False
            if cmd in (2, 17):
                C.cast(data, C.POINTER(C.c_bool))[0] = False
                return True
            return cmd in (8, 11, 16)  # performance hint, descriptors, legacy options
        @VIDEO
        def video(data, w, h, pitch):
            nonlocal pixels, width, height
            if data:
                if w > 4096 or h > 4096 or pitch < w * 2:
                    callback_errors.append('Invalid video dimensions')
                    return
                pixels = b''.join(C.string_at(data + y * pitch, w * 2) for y in range(h))
                width, height = w, h
        @AUDIO
        def audio_batch(data, count):
            audio.extend(C.string_at(data, count * 4))
            return count
        @POLL
        def poll():
            pass
        @INPUT
        def input_state(port, device, index, button):
            return int(port == 0 and device == 1 and index == 0 and button < 16 and bool(mask & (1 << button)))
        for name, callback in [('environment', environment), ('video_refresh', video), ('audio_sample_batch', audio_batch), ('input_poll', poll), ('input_state', input_state)]:
            func = getattr(core, 'retro_set_' + name)
            func.argtypes = [type(callback)]
            func(callback)
        core.retro_get_system_info.argtypes = [C.POINTER(Info)]
        core.retro_load_game.argtypes = [C.POINTER(Game)]
        core.retro_load_game.restype = C.c_bool
        core.retro_serialize_size.restype = C.c_size_t
        for name in ('retro_serialize', 'retro_unserialize'):
            func = getattr(core, name)
            func.argtypes = [C.c_void_p, C.c_size_t]
            func.restype = C.c_bool
        core.retro_init()
        info = Info()
        core.retro_get_system_info(C.byref(info))
        meta = dict(type='metadata', core=info.name.decode(), version=info.version.decode(), api=core.retro_api_version(), core_sha256=digest(core_path.read_bytes()), rtc_epoch=args.epoch, rtc_mode='frozen', timezone='UTC', pixel_format='RGB565', byteorder=sys.byteorder, input_sha256=digest(json.dumps(events, sort_keys=True).encode()), fps=60.25, sample_rate=44100)
        loaded = False
        try:
            if not args.rom:
                meta['game_checks'] = 'not_run_no_rom'
                print(json.dumps(meta, sort_keys=True))
                return
            rom = args.rom.read_bytes()
            rombuf = C.create_string_buffer(rom)
            game = Game(str(args.rom.resolve()).encode(), C.cast(rombuf, C.c_void_p), len(rom), None)
            if not core.retro_load_game(C.byref(game)):
                raise RuntimeError('Core rejected ROM')
            loaded = True
            meta.update(rom_sha256=digest(rom), rom_bytes=len(rom), game_checks='running')
            print(json.dumps(meta, sort_keys=True))
            def snapshot():
                size = core.retro_serialize_size()
                if size == 0:
                    raise RuntimeError('Core does not support serialization')
                state = C.create_string_buffer(size)
                if not core.retro_serialize(state, size):
                    raise RuntimeError('Serialization failed')
                return state.raw
            if args.snapshot_dir:
                args.snapshot_dir.mkdir(parents=True, exist_ok=True)
                export = core.retro_reference_snapshot
                export.argtypes = [C.c_void_p, C.c_size_t]
                export.restype = C.c_size_t
                core.retro_get_memory_data.argtypes = [C.c_uint]
                core.retro_get_memory_data.restype = C.c_void_p
                core.retro_get_memory_size.argtypes = [C.c_uint]
                core.retro_get_memory_size.restype = C.c_size_t
            if args.scene_check:
                scene_count = core.retro_reference_scene_count
                scene_count.argtypes = [C.c_uint]
                scene_count.restype = C.c_uint64
            def step(n):
                nonlocal mask
                mask = events.get(n, 0)
                audio.clear()
                core.retro_run()
                if callback_errors or pixels is None:
                    raise RuntimeError(callback_errors or 'No video frame')
                extra = {}
                if args.scene_check:
                    counts = [scene_count(i) for i in range(4)]
                    if counts[2] or counts[3]:
                        raise RuntimeError('Native scene mismatch or unsupported row: %s' % counts)
                    extra['scene_rows'] = counts[0]
                    extra['scene_pixels'] = counts[1]
                if args.snapshot_dir:
                    size = export(None, 0)
                    if size != 32876:
                        raise RuntimeError('Unexpected state export size')
                    capture = C.create_string_buffer(size)
                    # Insufficient capacity must be an inert rejection.
                    C.memset(capture, 0xa5, size)
                    if export(capture, size - 1) != 0 or capture.raw != bytes([0xa5]) * size:
                        raise RuntimeError('Non-atomic export rejection')
                    if export(capture, size) != size:
                        raise RuntimeError('State export failed')
                    blob = capture.raw
                    if export(capture, size) != size or blob != capture.raw:
                        raise RuntimeError('Export is not stable')
                    if core.retro_get_memory_size(2) != 16384:
                        raise RuntimeError('Unexpected system RAM size')
                    ram = C.string_at(core.retro_get_memory_data(2), 16384)
                    if blob[108:108+16384] != ram:
                        raise RuntimeError('Export differs from libretro RAM')
                    path = args.snapshot_dir / ('frame-%06d.ngr1' % n)
                    # Replay checks must not overwrite the first trace.
                    if path.exists():
                        if path.read_bytes() != blob:
                            raise RuntimeError('Existing snapshot differs')
                    else:
                        path.write_bytes(blob)
                    extra['cpu_ram_gpu_sha256'] = digest(blob)
                return dict(type='frame', frame=n, input_mask=mask, width=width, height=height, video_sha256=digest(pixels), audio_sha256=digest(audio), audio_samples=len(audio)//4, **extra)
            for frame in range(args.frames):
                print(json.dumps(step(frame), sort_keys=True))
            if args.scene_check and args.frames and scene_count(0) == 0:
                raise RuntimeError('Scene check ran no rows')
            state = snapshot()
            if args.state_out:
                args.state_out.write_bytes(state)
            print(json.dumps(dict(type='state', after_frames=args.frames, bytes=len(state), sha256=digest(state)), sort_keys=True))
            if args.replay_check:
                first = [step(n) for n in range(args.frames, args.frames + args.replay_check)]
                first_state = snapshot()
                saved = C.create_string_buffer(state)
                if not core.retro_unserialize(saved, len(state)):
                    raise RuntimeError('State restore rejected')
                second = [step(n) for n in range(args.frames, args.frames + args.replay_check)]
                second_state = snapshot()
                video_differences = [a['frame'] for a, b in zip(first, second)
                                     if (a['video_sha256'], a['width'], a['height']) !=
                                        (b['video_sha256'], b['width'], b['height'])]
                audio_differences = [a['frame'] for a, b in zip(first, second)
                                     if (a['audio_sha256'], a['audio_samples']) !=
                                        (b['audio_sha256'], b['audio_samples'])]
                ok = first == second
                print(json.dumps(dict(type='replay', frames=args.replay_check,
                    video_audio_match=ok, video_match=not video_differences,
                    audio_match=not audio_differences,
                    video_mismatch_frames=video_differences,
                    audio_mismatch_frames=audio_differences,
                    final_state_match=first_state == second_state,
                    first_final_state_sha256=digest(first_state),
                    restored_final_state_sha256=digest(second_state)), sort_keys=True))
                if not ok or first_state != second_state:
                    raise RuntimeError('Strict save-state replay mismatch; inspect separate video/audio/state results')
        finally:
            if loaded:
                core.retro_unload_game()
            core.retro_deinit()

if __name__ == '__main__':
    main()
