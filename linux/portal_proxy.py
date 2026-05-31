#!/usr/bin/env python3
"""
portal_proxy.py
Skylanders Portal proxy daemon for Steam Deck / Linux.
No pip required. Uses ctypes to call the system libusb directly.

Usage:
    python3 portal_proxy.py
    (then launch the game via Steam)
"""

import ctypes, socket, threading, sys, os, time, datetime

# ---------------------------------------------------------------------------
# File logger
# ---------------------------------------------------------------------------

_log_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "ssa-improved", "logs", "portal_proxy.log")
_log_lock = threading.Lock()

def _ts():
    return datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]

def plog(msg):
    line = f"[{_ts()}] {msg}"
    print(line, flush=True)
    with _log_lock:
        with open(_log_path, "a") as f:
            f.write(line + "\n")

# make sure the directory exists before opening it
os.makedirs(os.path.dirname(_log_path), exist_ok=True)

with open(_log_path, "w") as f:
    f.write(f"=== portal_proxy.py started {datetime.datetime.now()} ===\n")

# ---------------------------------------------------------------------------
# Load libusb
# ---------------------------------------------------------------------------

_lib = None
for _name in ('libusb-1.0.so.0', 'libusb-1.0.so', 'libusb.so.0'):
    try:
        _lib = ctypes.CDLL(_name)
        plog(f"Loaded {_name}")
        break
    except OSError:
        pass

if _lib is None:
    plog("ERROR: libusb-1.0 not found.")
    sys.exit(1)

# ---------------------------------------------------------------------------
# libusb constants
# ---------------------------------------------------------------------------

LIBUSB_SUCCESS       =  0
LIBUSB_ERROR_TIMEOUT = -7
BMRT_CLASS_IFACE_OUT = 0x21
EP_IN  = 0x81
IFACE  = 0
VID    = 0x1430
PID    = 0x0150
PORT   = 8764

# commands we log (R/S/A/LED - skip NFC block reads 0x51/0x57)
_LOG_CMDS = {0x52, 0x41, 0x53, 0x43}

# ---------------------------------------------------------------------------
# libusb function signatures
# ---------------------------------------------------------------------------

_lib.libusb_init.restype  = ctypes.c_int
_lib.libusb_init.argtypes = [ctypes.POINTER(ctypes.c_void_p)]

_lib.libusb_exit.restype  = None
_lib.libusb_exit.argtypes = [ctypes.c_void_p]

_lib.libusb_open_device_with_vid_pid.restype  = ctypes.c_void_p
_lib.libusb_open_device_with_vid_pid.argtypes = [ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16]
_lib.libusb_close.restype  = None
_lib.libusb_close.argtypes = [ctypes.c_void_p]

_lib.libusb_kernel_driver_active.restype  = ctypes.c_int
_lib.libusb_kernel_driver_active.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.libusb_detach_kernel_driver.restype  = ctypes.c_int
_lib.libusb_detach_kernel_driver.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.libusb_attach_kernel_driver.restype  = ctypes.c_int
_lib.libusb_attach_kernel_driver.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.libusb_claim_interface.restype  = ctypes.c_int
_lib.libusb_claim_interface.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.libusb_release_interface.restype  = ctypes.c_int
_lib.libusb_release_interface.argtypes = [ctypes.c_void_p, ctypes.c_int]

_lib.libusb_control_transfer.restype  = ctypes.c_int
_lib.libusb_control_transfer.argtypes = [
    ctypes.c_void_p,   # handle
    ctypes.c_uint8,    # bmRequestType
    ctypes.c_uint8,    # bRequest
    ctypes.c_uint16,   # wValue
    ctypes.c_uint16,   # wIndex
    ctypes.c_void_p,   # data (None for zero-length)
    ctypes.c_uint16,   # wLength
    ctypes.c_uint32,   # timeout ms
]

_lib.libusb_interrupt_transfer.restype  = ctypes.c_int
_lib.libusb_interrupt_transfer.argtypes = [
    ctypes.c_void_p,                 # handle
    ctypes.c_uint8,                  # endpoint
    ctypes.POINTER(ctypes.c_uint8),  # data buffer
    ctypes.c_int,                    # length
    ctypes.POINTER(ctypes.c_int),    # actual_length out
    ctypes.c_uint32,                 # timeout ms
]

# ---------------------------------------------------------------------------
# Device open / close
# ---------------------------------------------------------------------------

def open_portal():
    plog("Initialising libusb...")
    ctx = ctypes.c_void_p(None)
    r = _lib.libusb_init(ctypes.byref(ctx))
    if r != LIBUSB_SUCCESS:
        plog(f"libusb_init failed: {r}")
        return None, None

    plog("Looking for portal (VID=0x1430 PID=0x0150)...")
    handle = _lib.libusb_open_device_with_vid_pid(ctx, VID, PID)
    if not handle:
        plog("No physical portal found.")
        _lib.libusb_exit(ctx)
        return None, None
    plog(f"Portal opened: {handle:#x}")

    active = _lib.libusb_kernel_driver_active(handle, IFACE)
    if active == 1:
        r = _lib.libusb_detach_kernel_driver(handle, IFACE)
        plog(f"Detach kernel driver: {'OK' if r == 0 else f'FAILED ({r})'}")

    r = _lib.libusb_claim_interface(handle, IFACE)
    if r != LIBUSB_SUCCESS:
        plog(f"libusb_claim_interface failed: {r}")
        _lib.libusb_close(handle)
        _lib.libusb_exit(ctx)
        return None, None
    plog("Interface claimed")

    _lib.libusb_control_transfer(handle, BMRT_CLASS_IFACE_OUT, 0x0A, 0, IFACE, None, 0, 500)
    _lib.libusb_control_transfer(handle, BMRT_CLASS_IFACE_OUT, 0x0B, 1, IFACE, None, 0, 500)
    plog("Portal ready.\n")
    return ctx, handle


def close_portal(ctx, handle):
    if handle:
        _lib.libusb_release_interface(handle, IFACE)
        _lib.libusb_attach_kernel_driver(handle, IFACE)
        _lib.libusb_close(handle)
    if ctx:
        _lib.libusb_exit(ctx)
    plog("Portal closed, hid-generic reattached")

# ---------------------------------------------------------------------------
# Per-connection handler
# ---------------------------------------------------------------------------

def handle_client(conn, handle):
    stop = threading.Event()

    def read_loop():
        buf = (ctypes.c_uint8 * 32)()
        transferred = ctypes.c_int(0)
        time.sleep(0.1)
        while not stop.is_set():
            r = _lib.libusb_interrupt_transfer(
                handle, EP_IN,
                buf, 32,
                ctypes.byref(transferred),
                500)
            if stop.is_set():
                break
            if r == LIBUSB_SUCCESS and transferred.value > 0:
                pkt = bytes([0x02]) + bytes(buf[:transferred.value])
                pkt = pkt.ljust(33, b'\x00')[:33]
                cmd = pkt[1]
                if cmd in _LOG_CMDS:
                    plog(f"HW->DLL 0x{cmd:02X} [{pkt[2]:02X} {pkt[3]:02X} {pkt[4]:02X} {pkt[5]:02X}]")
                try:
                    conn.sendall(pkt)
                except OSError:
                    stop.set()
            elif r == LIBUSB_ERROR_TIMEOUT:
                pass
            elif r in (-1, -8):
                plog(f"Transient read error {r}, retrying...")
            else:
                plog(f"Fatal read error: {r}")
                stop.set()

    threading.Thread(target=read_loop, daemon=True).start()

    out_buf = (ctypes.c_uint8 * 32)()
    try:
        while not stop.is_set():
            # Receive exactly 33 bytes from DLL: 0x01 tag + 32 bytes command
            pkt = b''
            while len(pkt) < 33:
                try:
                    chunk = conn.recv(33 - len(pkt))
                except OSError:
                    return
                if not chunk:
                    return
                pkt += chunk

            cmd = pkt[1]
            if cmd in _LOG_CMDS:
                plog(f"DLL->HW 0x{cmd:02X} [{pkt[2]:02X} {pkt[3]:02X} {pkt[4]:02X} {pkt[5]:02X}]")

            if pkt[0] == 0x01:
                for i, b in enumerate(pkt[1:33]):
                    out_buf[i] = b
                _lib.libusb_control_transfer(
                    handle,
                    BMRT_CLASS_IFACE_OUT,
                    0x09,    # HID SET_REPORT
                    0x0200,  # Output report, report ID 0
                    IFACE,
                    out_buf, 32,
                    1000)
    except Exception as e:
        plog(f"Client handler error: {e}")
    finally:
        stop.set()

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    plog("=== Skylanders Spyro's Adventure Improved Portal Proxy ===")
    plog(f"Log: {_log_path}\n")

    ctx, handle = open_portal()
    if handle is None:
        plog("No physical portal found, exiting cleanly.")
        plog("If you are using the emulated portal, this is expected.")
        plog("If you intended to use a physical portal, check that it is plugged in.")
        sys.exit(0) # clean exit, not an error

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(('127.0.0.1', PORT))
    server.listen(1)
    plog(f"Listening on :{PORT} - launch the game now")

    try:
        while True:
            server.settimeout(1.0)
            try:
                conn, _ = server.accept()
            except socket.timeout:
                continue
            plog("DLL connected - portal active")
            handle_client(conn, handle)
            plog("DLL disconnected, waiting for reconnect...")
    except KeyboardInterrupt:
        plog("\nShutting down...")
    finally:
        server.close()
        close_portal(ctx, handle)

if __name__ == '__main__':
    main()