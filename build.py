#!/usr/bin/env python3
"""Build the Cydia repo into site/: debs, Packages(.gz/.bz2), Release, index.html.

Needs only Python 3 (no dpkg), so it runs the same on a Mac and in CI.
Each directory under packages/ is a package root: DEBIAN/ plus the files to install.
"""

import bz2
import gzip
import hashlib
import html
import io
import os
import shutil
import sys
import tarfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PKGS = ROOT / "packages"
SITE = ROOT / "site"
EPOCH = int(
    os.environ.get("SOURCE_DATE_EPOCH", "1793000000")
)  # fixed => reproducible debs


def parse_control(text):
    fields, last = {}, None
    for line in text.splitlines():
        if line.startswith((" ", "\t")) and last:
            fields[last] += "\n" + line
        elif ":" in line:
            last, _, v = line.partition(":")
            fields[last] = v.strip()
    return fields


def tar_gz(entries):
    """entries: list of (arcname, Path). Directories get 0755, files keep exec bit."""
    raw = io.BytesIO()
    with tarfile.open(fileobj=raw, mode="w", format=tarfile.GNU_FORMAT) as tf:
        for arc, path in entries:
            ti = tarfile.TarInfo(arc)
            ti.uid = ti.gid = 0
            ti.uname = ti.gname = "root"
            ti.mtime = EPOCH
            if path.is_dir():
                ti.type, ti.mode = tarfile.DIRTYPE, 0o755
                tf.addfile(ti)
            else:
                ti.size = path.stat().st_size
                ti.mode = 0o755 if os.access(path, os.X_OK) else 0o644
                with open(path, "rb") as f:
                    tf.addfile(ti, f)
    out = io.BytesIO()
    with gzip.GzipFile(fileobj=out, mode="wb", mtime=0, compresslevel=9) as g:
        g.write(raw.getvalue())
    return out.getvalue()


def ar_member(name, data):
    hdr = f"{name:<16}{EPOCH:<12}{0:<6}{0:<6}{'100644':<8}{len(data):<10}`\n".encode()
    assert len(hdr) == 60
    return hdr + data + (b"\n" if len(data) % 2 else b"")


def build_deb(pkgdir, outdir):
    debian = pkgdir / "DEBIAN"
    ctrl = parse_control((debian / "control").read_text())
    data_entries, size = [("./", pkgdir)], 0
    for p in sorted(pkgdir.rglob("*")):
        rel = p.relative_to(pkgdir)
        if rel.parts[0] in ("DEBIAN", ".DS_Store") or p.name == ".DS_Store":
            continue
        data_entries.append(("./" + str(rel) + ("/" if p.is_dir() else ""), p))
        if p.is_file():
            size += p.stat().st_size
    ctrl_text = (debian / "control").read_text().rstrip("\n")
    if "Installed-Size" not in ctrl:
        ctrl_text += f"\nInstalled-Size: {(size + 1023) // 1024}"
    ctrl_text += "\n"

    tmp = SITE / ".tmp-control"
    tmp.mkdir(parents=True, exist_ok=True)
    (tmp / "control").write_text(ctrl_text)
    ctrl_entries = [("./", tmp), ("./control", tmp / "control")]
    for name in ("preinst", "postinst", "prerm", "postrm", "conffiles"):
        if (debian / name).exists():
            ctrl_entries.append(("./" + name, debian / name))
    control_tgz = tar_gz(ctrl_entries)
    data_tgz = tar_gz(data_entries)
    shutil.rmtree(tmp)

    deb = (
        b"!<arch>\n"
        + ar_member("debian-binary", b"2.0\n")
        + ar_member("control.tar.gz", control_tgz)
        + ar_member("data.tar.gz", data_tgz)
    )
    fname = f"{ctrl['Package']}_{ctrl['Version']}_{ctrl['Architecture']}.deb"
    (outdir / fname).write_bytes(deb)
    return fname, ctrl_text, deb


def digests(data):
    return {k: hashlib.new(k, data).hexdigest() for k in ("md5", "sha1", "sha256")}


def main():
    if SITE.exists():
        shutil.rmtree(SITE)
    debs = SITE / "debs"
    debs.mkdir(parents=True)
    stanzas, rows = [], []
    for pkgdir in sorted(
        p for p in PKGS.iterdir() if (p / "DEBIAN" / "control").exists()
    ):
        fname, ctrl_text, deb = build_deb(pkgdir, debs)
        d = digests(deb)
        stanzas.append(
            ctrl_text + f"Filename: debs/{fname}\nSize: {len(deb)}\n"
            f"MD5sum: {d['md5']}\nSHA1: {d['sha1']}\nSHA256: {d['sha256']}\n"
        )
        c = parse_control(ctrl_text)
        rows.append(
            (
                c.get("Name", c["Package"]),
                c["Package"],
                c["Version"],
                c["Description"].splitlines()[0],
                f"debs/{fname}",
            )
        )
        print("built", fname, len(deb), "bytes")

    packages = ("\n".join(stanzas)).encode()
    (SITE / "Packages").write_bytes(packages)
    (SITE / "Packages.gz").write_bytes(gzip.compress(packages, 9, mtime=0))
    (SITE / "Packages.bz2").write_bytes(bz2.compress(packages, 9))

    lines = [
        "Origin: tslmy",
        "Label: tslmy's legacy iOS repo",
        "Suite: stable",
        "Version: 1.0",
        "Codename: ios",
        "Architectures: iphoneos-arm",
        "Components: main",
        "Description: Packages for jailbroken iOS 6 devices"
        f"\nDate: {time.strftime('%a, %d %b %Y %H:%M:%S UTC', time.gmtime())}",
    ]
    for algo, key in (("MD5Sum", "md5"), ("SHA1", "sha1"), ("SHA256", "sha256")):
        lines.append(f"{algo}:")
        for n in ("Packages", "Packages.gz", "Packages.bz2"):
            data = (SITE / n).read_bytes()
            lines.append(f" {digests(data)[key]} {len(data):>8} {n}")
    (SITE / "Release").write_text("\n".join(lines) + "\n")

    items = "\n".join(
        f"<li><b>{html.escape(n)}</b> <code>{html.escape(p)} {html.escape(v)}</code><br>"
        f'{html.escape(d)} (<a href="{f}">.deb</a>)</li>'
        for n, p, v, d, f in rows
    )
    (SITE / "index.html").write_text(
        "<!doctype html><meta charset=utf-8><meta name=viewport content='width=device-width'>"
        "<title>Legacy iOS repo</title><h1>Legacy iOS repo</h1>"
        "<p>Add this URL as a source in Cydia (iOS 6 armv7):</p><pre id=u></pre>"
        "<script>u.textContent=location.href.replace(/index\\.html$/,'')</script>"
        f"<ul>{items}</ul>"
    )
    (SITE / ".nojekyll").write_text("")
    print("site/ ready")


if __name__ == "__main__":
    sys.exit(main())
