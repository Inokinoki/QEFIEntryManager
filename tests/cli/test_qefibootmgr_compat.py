#!/usr/bin/env python3
"""Behavior tests comparing qefibootmgr with RedHat's efibootmgr.

Issue: https://github.com/Inokinoki/QEFIEntryManager/issues/48

The suite checks:
  1. CLI option surface against rhboot/efibootmgr (core flags must exist).
  2. Listing format used by rhboot/efibootmgr (BootNext/BootCurrent/Timeout/
     BootOrder/BootXXXX* lines).
  3. Mutations (timeout, boot order, BootNext, active/inactive, delete,
     remove-dups, quiet, create, driver/sysprep, unicode, mirror) against a
     dummy EFI-variable backend.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import unittest
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Set, Tuple

EFI_GLOBAL_GUID = "8be4df61-93ca-11d2-aa0d-00e098032b8c"

# Core long options shared with rhboot/efibootmgr. These must appear in
# `qefibootmgr --help`. Keep in sync with redhat_efibootmgr_options.txt.
CORE_LONG_OPTIONS = {
    "active",
    "inactive",
    "bootnum",
    "delete-bootnum",
    "create",
    "create-only",
    "disk",
    "remove-dups",
    "loader",
    "label",
    "bootnext",
    "delete-bootnext",
    "bootorder",
    "delete-bootorder",
    "part",
    "quiet",
    "timeout",
    "delete-timeout",
    "verbose",
    "version",
    "help",
    "edd",
    "device",
    "full-dev-path",
    "file-dev-path",
    "reconnect",
    "no-reconnect",
    "gpt",
    "iface",
    "index",
    "mirror-below-4G",
    "mirror-above-4G",
    "driver",
    "unicode",
    "UCS-2",
    "sysprep",
    "append-binary-args",
    "write-signature",
}

CORE_SHORT_OPTIONS = set("aAbBcCdDeEfFgIiIlLmMnNoOpqrtTuvVwyh@")

OPTIONAL_REDHAT_OPTIONS: Set[str] = set()

BOOT_ENTRY_RE = re.compile(r"^Boot([0-9A-Fa-f]{4})([* ]) (.*)$")
LONG_OPT_RE = re.compile(r"--([A-Za-z0-9][A-Za-z0-9_-]*)")
SHORT_OPT_RE = re.compile(r"(?:^|[\s,|])-([A-Za-z@])(?:[\s,|]|$)")


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _options_file() -> Path:
    return Path(__file__).resolve().parent / "redhat_efibootmgr_options.txt"


def load_option_table(path: Optional[Path] = None) -> Tuple[Set[str], Set[str]]:
    path = path or _options_file()
    core: Set[str] = set()
    optional: Set[str] = set()
    if not path.is_file():
        return set(CORE_LONG_OPTIONS), set(OPTIONAL_REDHAT_OPTIONS)
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        kind, name = line.split(None, 1)
        if kind == "core":
            core.add(name)
        elif kind == "optional":
            optional.add(name)
    return core or set(CORE_LONG_OPTIONS), optional or set(OPTIONAL_REDHAT_OPTIONS)


def make_load_option(description: str, file_path: str = "", active: bool = True) -> bytes:
    """Build a minimal EFI_LOAD_OPTION blob (little-endian)."""
    attributes = struct.pack("<I", 0x00000001 if active else 0x00000000)
    desc = description.encode("utf-16le") + b"\x00\x00"
    if file_path:
        path_ucs2 = file_path.encode("utf-16le") + b"\x00\x00"
        file_dp = bytes([0x04, 0x04]) + struct.pack("<H", 4 + len(path_ucs2)) + path_ucs2
        end_dp = bytes([0x7F, 0xFF, 0x04, 0x00])
        device_path = file_dp + end_dp
    else:
        device_path = bytes([0x7F, 0xFF, 0x04, 0x00])
    return attributes + struct.pack("<H", len(device_path)) + desc + device_path


def uint16_le(value: int) -> bytes:
    return struct.pack("<H", value & 0xFFFF)


def bootorder_bytes(ids: Sequence[int]) -> bytes:
    return b"".join(uint16_le(i) for i in ids)


@dataclass
class BootEntry:
    boot_id: str
    active: bool
    name: str
    path: str = ""


@dataclass
class BootListing:
    bootnext: Optional[str] = None
    bootcurrent: Optional[str] = None
    timeout: Optional[int] = None
    bootorder: Optional[List[str]] = None
    no_bootorder: bool = False
    entries: List[BootEntry] = field(default_factory=list)


def parse_listing(text: str) -> BootListing:
    """Extract rhboot/efibootmgr-style listing lines, ignoring extra status text."""
    listing = BootListing()
    for raw in text.splitlines():
        line = raw.strip("\r")
        if line.startswith("BootNext:"):
            listing.bootnext = line.split(":", 1)[1].strip().upper()
        elif line.startswith("BootCurrent:"):
            listing.bootcurrent = line.split(":", 1)[1].strip().upper()
        elif line.startswith("Timeout:"):
            match = re.search(r"(\d+)", line)
            if match:
                listing.timeout = int(match.group(1))
        elif line.startswith("BootOrder:"):
            value = line.split(":", 1)[1].strip()
            listing.bootorder = [part.strip().upper() for part in value.split(",") if part.strip()]
        elif line.startswith("No BootOrder is set"):
            listing.no_bootorder = True
            listing.bootorder = None
        else:
            match = BOOT_ENTRY_RE.match(line)
            if match:
                name_and_path = match.group(3)
                name, path = name_and_path, ""
                if "\t" in name_and_path:
                    name, path = name_and_path.split("\t", 1)
                listing.entries.append(
                    BootEntry(
                        boot_id=match.group(1).upper(),
                        active=match.group(2) == "*",
                        name=name,
                        path=path,
                    )
                )
    return listing


def extract_options(help_text: str) -> Tuple[Set[str], Set[str]]:
    long_opts = set(LONG_OPT_RE.findall(help_text))
    short_opts = set(SHORT_OPT_RE.findall(help_text))
    return long_opts, short_opts


class QEFIBootMgr:
    def __init__(self, binary: Path, home: Path):
        self.binary = binary
        self.home = home
        self.xdg_data_home = home / ".local" / "share"
        self.xdg_data_home.mkdir(parents=True, exist_ok=True)
        self.env = os.environ.copy()
        self.env["HOME"] = str(home)
        self.env["XDG_DATA_HOME"] = str(self.xdg_data_home)
        self.env.setdefault("LANG", "C")
        self.env.setdefault("LC_ALL", "C")
        # Avoid Qt plugin / display issues in headless CI.
        self.env.setdefault("QT_QPA_PLATFORM", "offscreen")
        self.env.setdefault("QT_LOGGING_RULES", "*.debug=false")

    def dummy_dirs(self) -> List[Path]:
        names = ("qefibootmgr", "QEFIEntryManager", "efibootmgr")
        return [self.xdg_data_home / name for name in names] + [
            self.home / ".local" / "share" / name for name in names
        ]

    def var_filename(self, name: str) -> str:
        return f"{EFI_GLOBAL_GUID}{name}.bin"

    def write_var(self, name: str, data: bytes) -> None:
        for directory in self.dummy_dirs():
            directory.mkdir(parents=True, exist_ok=True)
            (directory / self.var_filename(name)).write_bytes(data)

    def read_var(self, name: str) -> Optional[bytes]:
        for directory in self.dummy_dirs():
            path = directory / self.var_filename(name)
            if path.is_file():
                return path.read_bytes()
        return None

    def seed_default_entries(self) -> None:
        entries = {
            "Boot0000": make_load_option("Linux", r"\EFI\BOOT\BOOTX64.EFI", active=True),
            "Boot0001": make_load_option(
                "rEFInd Boot Manager", r"\EFI\refind\refind_x64.efi", active=True
            ),
            "Boot0002": make_load_option(
                "Windows Boot Manager", r"\EFI\Microsoft\Boot\bootmgfw.efi", active=False
            ),
        }
        for name, blob in entries.items():
            self.write_var(name, blob)
        self.write_var("BootOrder", bootorder_bytes([0x0000, 0x0001, 0x0002]))
        self.write_var("Timeout", uint16_le(30))
        self.write_var("BootCurrent", uint16_le(0x0000))
        # Ensure BootNext starts unset (empty file is treated as missing).
        for directory in self.dummy_dirs():
            next_path = directory / self.var_filename("BootNext")
            if next_path.exists():
                next_path.unlink()

    def run(self, *args: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            [str(self.binary), *args],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=self.env,
            text=True,
            check=False,
        )


def listing_header_order(text: str) -> List[str]:
    order = []
    for raw in text.splitlines():
        line = raw.strip()
        if line.startswith("BootNext:"):
            order.append("BootNext")
        elif line.startswith("BootCurrent:"):
            order.append("BootCurrent")
        elif line.startswith("Timeout:"):
            order.append("Timeout")
        elif line.startswith("BootOrder:") or line.startswith("No BootOrder is set"):
            order.append("BootOrder")
        elif BOOT_ENTRY_RE.match(line):
            break
    return order


class CompatTests(unittest.TestCase):
    binary: Path
    live_efibootmgr: Optional[str]

    @classmethod
    def setUpClass(cls) -> None:
        if not cls.binary.is_file():
            raise unittest.SkipTest(f"qefibootmgr binary not found: {cls.binary}")

    def setUp(self) -> None:
        self.tmpdir = Path(tempfile.mkdtemp(prefix="qefibootmgr-compat-"))
        self.cli = QEFIBootMgr(self.binary, self.tmpdir)
        self.cli.seed_default_entries()

    def tearDown(self) -> None:
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def run_ok(self, *args: str) -> subprocess.CompletedProcess:
        result = self.cli.run(*args)
        if result.returncode != 0:
            self.fail(
                f"qefibootmgr {' '.join(args)} failed ({result.returncode})\n"
                f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
            )
        return result

    def listing(self, *args: str) -> Tuple[BootListing, str]:
        result = self.run_ok(*args)
        return parse_listing(result.stdout), result.stdout

    def dummy_backend_available(self) -> bool:
        result = self.cli.run()
        if result.returncode != 0:
            return False
        listing = parse_listing(result.stdout)
        return any(entry.boot_id == "0000" for entry in listing.entries)

    def test_help_lists_redhat_core_options(self) -> None:
        result = self.run_ok("--help")
        long_opts, short_opts = extract_options(result.stdout)
        core, _optional = load_option_table()
        missing_long = sorted(core - long_opts)
        self.assertFalse(
            missing_long,
            f"qefibootmgr --help is missing RedHat core options: {missing_long}\n{result.stdout}",
        )
        missing_short = sorted(CORE_SHORT_OPTIONS - short_opts)
        self.assertFalse(
            missing_short,
            f"qefibootmgr --help is missing RedHat short options: {missing_short}\n{result.stdout}",
        )

    def test_help_matches_live_redhat_efibootmgr_core_options(self) -> None:
        if not self.live_efibootmgr:
            self.skipTest("system efibootmgr not installed")
        redhat = subprocess.run(
            [self.live_efibootmgr, "-h"],
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        ours = self.run_ok("--help")
        redhat_long, _redhat_short = extract_options(redhat.stdout)
        our_long, _our_short = extract_options(ours.stdout)
        core, _optional = load_option_table()
        expected = (redhat_long & core) or core
        missing = sorted(expected - our_long)
        self.assertFalse(
            missing,
            "qefibootmgr is missing core options advertised by the installed "
            f"efibootmgr ({self.live_efibootmgr}): {missing}",
        )

    def test_version_flag(self) -> None:
        for flag in ("-V", "--version"):
            result = self.run_ok(flag)
            self.assertRegex(
                result.stdout,
                r"[Vv]ersion",
                f"{flag} should print a version string, got: {result.stdout!r}",
            )

    def test_default_listing_matches_redhat_format(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing()
        self.assertEqual(
            listing_header_order(stdout),
            ["BootCurrent", "Timeout", "BootOrder"],
            f"listing header order should match rhboot/efibootmgr, got:\n{stdout}",
        )
        self.assertIsNone(listing.bootnext)
        self.assertEqual(listing.bootcurrent, "0000")
        self.assertEqual(listing.timeout, 30)
        self.assertEqual(listing.bootorder, ["0000", "0001", "0002"])
        self.assertEqual([entry.boot_id for entry in listing.entries], ["0000", "0001", "0002"])
        by_id: Dict[str, BootEntry] = {entry.boot_id: entry for entry in listing.entries}
        self.assertTrue(by_id["0000"].active)
        self.assertEqual(by_id["0000"].name, "Linux")
        self.assertTrue(by_id["0001"].active)
        self.assertEqual(by_id["0001"].name, "rEFInd Boot Manager")
        self.assertFalse(by_id["0002"].active)
        self.assertEqual(by_id["0002"].name, "Windows Boot Manager")
        # Inactive entries use a space where active entries use '*', like efibootmgr.
        self.assertRegex(stdout, re.compile(r"^Boot0002  Windows Boot Manager$", re.MULTILINE))

    def test_verbose_includes_loader_path(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing("-v")
        by_id = {entry.boot_id: entry for entry in listing.entries}
        self.assertIn(r"\EFI\refind\refind_x64.efi", by_id["0001"].path)
        self.assertIn("\t", stdout)

    def test_quiet_suppresses_listing(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.run_ok("-q")
        self.assertEqual(result.stdout.strip(), "")
        result = self.run_ok("-q", "-t", "5")
        self.assertEqual(result.stdout.strip(), "")

    def test_timeout_set_and_delete(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, _stdout = self.listing("-t", "10")
        self.assertEqual(listing.timeout, 10)
        listing, stdout = self.listing("-T")
        self.assertIsNone(listing.timeout, f"Timeout should be omitted after -T:\n{stdout}")

    def test_bootorder_set(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, _stdout = self.listing("-o", "0002,0000,0001")
        self.assertEqual(listing.bootorder, ["0002", "0000", "0001"])
        listing, _stdout = self.listing("--bootorder", "0001,0000")
        self.assertEqual(listing.bootorder, ["0001", "0000"])

    def test_delete_bootorder_message(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing("-O")
        self.assertTrue(listing.no_bootorder, f"expected RedHat empty-BootOrder message:\n{stdout}")
        self.assertIn("No BootOrder is set; firmware will attempt recovery", stdout)

    def test_bootnext_set_and_delete(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing("-n", "0001")
        self.assertEqual(listing.bootnext, "0001")
        self.assertEqual(
            listing_header_order(stdout)[:1],
            ["BootNext"],
            f"BootNext should be printed first, like efibootmgr:\n{stdout}",
        )
        listing, stdout = self.listing("-N")
        self.assertIsNone(listing.bootnext, f"BootNext should be omitted after -N:\n{stdout}")

    def test_active_and_inactive(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, _stdout = self.listing("-b", "0002", "-a")
        by_id = {entry.boot_id: entry for entry in listing.entries}
        self.assertTrue(by_id["0002"].active)
        listing, stdout = self.listing("-b", "0000", "-A")
        by_id = {entry.boot_id: entry for entry in listing.entries}
        self.assertFalse(by_id["0000"].active)
        self.assertRegex(stdout, re.compile(r"^Boot0000  Linux$", re.MULTILINE))

    def test_delete_bootnum(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing("-b", "0001", "-B")
        self.assertEqual(listing.bootorder, ["0000", "0002"])
        self.assertNotIn("0001", [entry.boot_id for entry in listing.entries])
        entry_lines = [line for line in stdout.splitlines() if BOOT_ENTRY_RE.match(line)]
        self.assertFalse(any(line.startswith("Boot0001") for line in entry_lines), stdout)

    def test_remove_dups(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        self.cli.write_var("BootOrder", bootorder_bytes([0x0000, 0x0001, 0x0000, 0x0002, 0x0001]))
        listing, _stdout = self.listing("-D")
        self.assertEqual(listing.bootorder, ["0000", "0001", "0002"])

    def test_delete_bootnum_requires_bootnum(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-B")
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stderr, r"bootnum", re.IGNORECASE)

    def test_active_requires_bootnum(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-a")
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stderr, r"bootnum", re.IGNORECASE)

    def test_active_and_inactive_conflict(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-b", "0000", "-a", "-A")
        self.assertNotEqual(result.returncode, 0)

    def test_invalid_bootorder_is_error(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-o", "zzzz")
        self.assertNotEqual(result.returncode, 0)

    def test_create_file_dev_path_adds_to_bootorder(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing(
            "-c", "--file-dev-path", "-L", "Test OS", "-l", r"\EFI\test\test.efi"
        )
        self.assertIn("0003", listing.bootorder)
        self.assertEqual(listing.bootorder[0], "0003")
        by_id = {entry.boot_id: entry for entry in listing.entries}
        self.assertIn("0003", by_id)
        self.assertEqual(by_id["0003"].name, "Test OS")
        self.assertTrue(by_id["0003"].active)
        self.assertRegex(stdout, re.compile(r"^Boot0003\* Test OS$", re.MULTILINE))

    def test_create_only_skips_bootorder(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, _stdout = self.listing(
            "-C", "--file-dev-path", "-L", "Recovery", "-b", "000A"
        )
        self.assertEqual(listing.bootorder, ["0000", "0001", "0002"])
        self.assertIn("000A", [entry.boot_id for entry in listing.entries])

    def test_create_index_inserts_into_bootorder(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, _stdout = self.listing(
            "-c", "--file-dev-path", "-I", "1", "-L", "Middle"
        )
        self.assertEqual(listing.bootorder[1], "0003")

    def test_create_with_unicode_optional_data(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.run_ok(
            "-C", "--file-dev-path", "-L", "UKI", "-u", r"\kernel.efi"
        )
        listing = parse_listing(result.stdout)
        self.assertIn("0003", [entry.boot_id for entry in listing.entries])
        raw = self.cli.read_var("Boot0003")
        self.assertIsNotNone(raw)
        self.assertIn("k\x00e\x00r\x00n\x00e\x00l".encode("ascii"), raw)

    def test_create_with_append_binary_args(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        extra = self.tmpdir / "extra.bin"
        extra.write_bytes(b"OPTDATA")
        listing, _stdout = self.listing(
            "-C", "--file-dev-path", "-L", "WithArgs", "-@", str(extra)
        )
        self.assertIn("0003", [entry.boot_id for entry in listing.entries])
        raw = self.cli.read_var("Boot0003")
        self.assertIsNotNone(raw)
        self.assertIn(b"OPTDATA", raw)

    def test_driver_mode_lists_driver_order(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        self.cli.write_var("Driver0000", make_load_option("NVMe Driver", r"\EFI\drivers\nvme.efi"))
        self.cli.write_var("DriverOrder", bootorder_bytes([0x0000]))
        result = self.run_ok("-r")
        self.assertIn("DriverOrder: 0000", result.stdout)
        self.assertRegex(result.stdout, re.compile(r"^Driver0000\* NVMe Driver$", re.MULTILINE))
        self.assertNotIn("BootOrder:", result.stdout)

    def test_sysprep_rejects_bootnext(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-y", "-n", "0000")
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stderr, r"BootNext")

    def test_reconnect_requires_driver_mode(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        result = self.cli.run("-f", "-b", "0000")
        self.assertNotEqual(result.returncode, 0)
        self.assertRegex(result.stderr, r"reconnect", re.IGNORECASE)

    def test_mirror_request_roundtrip(self) -> None:
        if not self.dummy_backend_available():
            self.skipTest("dummy EFI backend is not available (need APP_DATA_DUMMY_BACKEND)")
        listing, stdout = self.listing("-m", "t", "-M", "50")
        self.assertIn("RequestMirrorMemoryBelow4GB: true", stdout)
        self.assertIn("RequestMirroredPercentageAbove4G: 50.00", stdout)

    def test_report_redhat_only_option_gaps(self) -> None:
        result = self.run_ok("--help")
        long_opts, _short = extract_options(result.stdout)
        core, optional = load_option_table()
        missing_core = sorted(core - long_opts)
        self.assertFalse(missing_core, f"missing RedHat options: {missing_core}\n{result.stdout}")
        missing_optional = sorted(optional - long_opts)
        print(
            "RedHat-only options not implemented by qefibootmgr: "
            + (", ".join(missing_optional) if missing_optional else "(none)")
        )


def parse_args(argv: Sequence[str]) -> Tuple[argparse.Namespace, List[str]]:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--qefibootmgr",
        required=True,
        help="Path to the qefibootmgr binary under test",
    )
    parser.add_argument(
        "--efibootmgr",
        default=shutil.which("efibootmgr"),
        help="Optional path to RedHat/system efibootmgr for live option comparison",
    )
    return parser.parse_known_args(argv)


def print_efibootmgr_version(path: Optional[str]) -> None:
    if not path:
        print("Live efibootmgr: (not found)", flush=True)
        return
    version = subprocess.run(
        [path, "--version"],
        capture_output=True,
        text=True,
        check=False,
    )
    output = (version.stdout or version.stderr).strip() or f"exit {version.returncode}"
    print(f"Live efibootmgr: {path}", flush=True)
    print(f"Live efibootmgr version: {output}", flush=True)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args, rest = parse_args(argv or sys.argv[1:])
    CompatTests.binary = Path(args.qefibootmgr).resolve()
    CompatTests.live_efibootmgr = args.efibootmgr
    print_efibootmgr_version(args.efibootmgr)
    unittest_args = [sys.argv[0], *rest]
    result = unittest.main(argv=unittest_args, exit=False, verbosity=2)
    return 0 if result.result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
