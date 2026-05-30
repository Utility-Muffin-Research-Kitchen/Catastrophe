#!/usr/bin/env python3
"""Generate Catastrophe's status/control asset atlas.

The original development preview atlas came from NextUI. This generator keeps
the same 1x sprite coordinates, but redraws the sprites from simple procedural
geometry and Tabler-icon-derived glyphs so the generated PNGs are redistributable.
"""

from __future__ import annotations

import argparse
import math
import re
import struct
import zlib
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Iterable


ATLAS_SIZE = 128
SCALES = (1, 2, 3, 4)
SUPERSAMPLE = 4

WHITE = (255, 255, 255, 255)
BLACK = (0, 0, 0, 255)
DARK_GRAY = (38, 38, 38, 255)
LIGHT_GRAY = (167, 167, 167, 255)
MID_GRAY = (127, 127, 127, 255)
RED = (255, 0, 0, 255)


@dataclass(frozen=True)
class Asset:
    name: str
    rect: tuple[float, float, float, float]
    source: str


ASSET_MANIFEST: tuple[Asset, ...] = (
    Asset("ASSET_WHITE_PILL", (1, 1, 30, 30), "procedural circle/pill cap"),
    Asset("ASSET_BLACK_PILL", (33, 1, 30, 30), "procedural circle/pill cap"),
    Asset("ASSET_DARK_GRAY_PILL", (65, 1, 30, 30), "procedural circle/pill cap"),
    Asset("ASSET_OPTION", (97, 1, 20, 20), "procedural option dot"),
    Asset("ASSET_BUTTON", (1, 33, 20, 20), "procedural button circle"),
    Asset("ASSET_PAGE_BG", (64, 33, 15, 15), "procedural page dot"),
    Asset("ASSET_STATE_BG", (23, 54, 8, 8), "procedural state dot"),
    Asset("ASSET_PAGE", (39, 54, 6, 6), "procedural page dot"),
    Asset("ASSET_BAR", (33, 58, 4, 4), "procedural rounded bar segment"),
    Asset("ASSET_BAR_BG", (33, 58, 4, 4), "same rect as ASSET_BAR"),
    Asset("ASSET_BAR_BG_MENU", (33, 58, 4, 4), "same rect as ASSET_BAR"),
    Asset("ASSET_UNDERLINE", (85, 51, 3, 3), "procedural underline dot"),
    Asset("ASSET_DOT", (33, 54, 2, 2), "procedural dot"),
    Asset("ASSET_HOLE", (1, 63, 20, 20), "procedural button hole"),
    Asset("ASSET_BRIGHTNESS", (1, 85, 19, 19), "Tabler brightness/sun"),
    Asset("ASSET_COLORTEMP", (41, 85, 9, 19), "Tabler temperature"),
    Asset("ASSET_VOLUME_MUTE", (21, 85, 10, 19), "Tabler volume"),
    Asset("ASSET_VOLUME", (21, 85, 19, 19), "Tabler volume-2"),
    Asset("ASSET_BATTERY", (47, 51, 17, 10), "Tabler battery, fitted to NextUI geometry"),
    Asset("ASSET_BATTERY_LOW", (66, 51, 17, 10), "Tabler battery, low/red"),
    Asset("ASSET_BATTERY_FILL", (81, 33, 12, 6), "battery fill block"),
    Asset("ASSET_BATTERY_FILL_LOW", (1, 55, 12, 6), "battery fill block, low/red"),
    Asset("ASSET_BATTERY_BOLT", (81, 41, 12, 6), "Tabler bolt, fitted to battery"),
    Asset("ASSET_SCROLL_UP", (97, 23, 24, 6), "Tabler chevron-up"),
    Asset("ASSET_SCROLL_DOWN", (97, 31, 24, 6), "Tabler chevron-down"),
    Asset("ASSET_WIFI", (1, 104, 12, 12), "Tabler wifi"),
    Asset("ASSET_WIFI_MED", (14, 104, 12, 12), "Tabler wifi, medium signal"),
    Asset("ASSET_WIFI_LOW", (27, 104, 12, 12), "Tabler wifi, low signal"),
    Asset("ASSET_WIFI_OFF", (40, 104, 12, 12), "Tabler wifi-off"),
    Asset("ASSET_BLUETOOTH", (53, 104, 12, 12), "Tabler bluetooth"),
    Asset("ASSET_BLUETOOTH_OFF", (66, 104, 12, 12), "Tabler bluetooth-off"),
    Asset("ASSET_AUDIO", (79, 104, 12, 12), "Tabler headphones, filled"),
    Asset("ASSET_CONTROLLER", (92, 104, 12, 12), "Tabler device-gamepad-2, filled"),
    Asset("ASSET_CHECKCIRCLE", (1, 117, 10, 10), "Tabler circle-check"),
    Asset("ASSET_LOCK", (12, 116, 8, 11), "Tabler lock"),
    Asset("ASSET_SETTINGS", (21, 117, 10, 10), "Tabler settings"),
    Asset("ASSET_STORE", (66, 117, 10, 10), "Tabler cloud-download"),
    Asset("ASSET_GAMEPAD", (91, 51, 17, 10), "Tabler device-gamepad, filled"),
    Asset("ASSET_POWEROFF", (43, 117, 10, 10), "Tabler power"),
    Asset("ASSET_RESTART", (54, 119, 11, 8), "Tabler refresh, filled-style"),
    Asset("ASSET_SUSPEND", (32, 117, 10, 10), "Tabler moon"),
    Asset("LEGACY_WIFI_SMALL", (96, 40, 12, 8), "legacy unmapped cluster"),
    Asset("LEGACY_BLACK_DOT", (15, 55, 4, 4), "legacy unmapped cluster"),
    Asset("LEGACY_DARK_DOT", (85, 56, 4, 4), "legacy unmapped cluster"),
)


class Canvas:
    def __init__(self, scale: int, supersample: int = SUPERSAMPLE):
        self.scale = scale
        self.supersample = supersample
        self.width = ATLAS_SIZE * scale
        self.height = ATLAS_SIZE * scale
        size = self.width * self.height
        self.a = [0.0] * size
        self.r = [0.0] * size
        self.g = [0.0] * size
        self.b = [0.0] * size

    def _blend(self, idx: int, color: tuple[int, int, int, int], coverage: float) -> None:
        if coverage <= 0.0:
            return
        sa = coverage * (color[3] / 255.0)
        if sa <= 0.0:
            return
        inv = 1.0 - sa
        self.r[idx] = (color[0] / 255.0) * sa + self.r[idx] * inv
        self.g[idx] = (color[1] / 255.0) * sa + self.g[idx] * inv
        self.b[idx] = (color[2] / 255.0) * sa + self.b[idx] * inv
        self.a[idx] = sa + self.a[idx] * inv

    def _erase(self, idx: int, coverage: float) -> None:
        if coverage <= 0.0:
            return
        keep = 1.0 - min(1.0, coverage)
        self.r[idx] *= keep
        self.g[idx] *= keep
        self.b[idx] *= keep
        self.a[idx] *= keep

    def _paint(
        self,
        bbox: tuple[float, float, float, float],
        color: tuple[int, int, int, int],
        contains: Callable[[float, float], bool],
        erase: bool = False,
    ) -> None:
        min_x, min_y, max_x, max_y = bbox
        px0 = max(0, int(math.floor(min_x * self.scale)) - 1)
        py0 = max(0, int(math.floor(min_y * self.scale)) - 1)
        px1 = min(self.width, int(math.ceil(max_x * self.scale)) + 1)
        py1 = min(self.height, int(math.ceil(max_y * self.scale)) + 1)
        ss = self.supersample
        denom = ss * ss
        for py in range(py0, py1):
            for px in range(px0, px1):
                hits = 0
                for sy in range(ss):
                    y = (py + (sy + 0.5) / ss) / self.scale
                    for sx in range(ss):
                        x = (px + (sx + 0.5) / ss) / self.scale
                        if contains(x, y):
                            hits += 1
                if hits:
                    idx = py * self.width + px
                    coverage = hits / denom
                    if erase:
                        self._erase(idx, coverage)
                    else:
                        self._blend(idx, color, coverage)

    def circle(self, cx: float, cy: float, radius: float, color: tuple[int, int, int, int]) -> None:
        rr = radius * radius
        self._paint(
            (cx - radius, cy - radius, cx + radius, cy + radius),
            color,
            lambda x, y: (x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr,
        )

    def erase_circle(self, cx: float, cy: float, radius: float) -> None:
        rr = radius * radius
        self._paint(
            (cx - radius, cy - radius, cx + radius, cy + radius),
            (0, 0, 0, 255),
            lambda x, y: (x - cx) * (x - cx) + (y - cy) * (y - cy) <= rr,
            erase=True,
        )

    def rect(self, x: float, y: float, w: float, h: float, color: tuple[int, int, int, int]) -> None:
        self._paint((x, y, x + w, y + h), color, lambda px, py: x <= px <= x + w and y <= py <= y + h)

    def erase_rect(self, x: float, y: float, w: float, h: float) -> None:
        self._paint(
            (x, y, x + w, y + h),
            (0, 0, 0, 255),
            lambda px, py: x <= px <= x + w and y <= py <= y + h,
            erase=True,
        )

    def rounded_rect(
        self,
        x: float,
        y: float,
        w: float,
        h: float,
        radius: float,
        color: tuple[int, int, int, int],
    ) -> None:
        r = min(radius, w / 2.0, h / 2.0)
        rr = r * r

        def contains(px: float, py: float) -> bool:
            if not (x <= px <= x + w and y <= py <= y + h):
                return False
            cx = min(max(px, x + r), x + w - r)
            cy = min(max(py, y + r), y + h - r)
            return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= rr

        self._paint((x, y, x + w, y + h), color, contains)

    def line(
        self,
        x1: float,
        y1: float,
        x2: float,
        y2: float,
        width: float,
        color: tuple[int, int, int, int],
        square_caps: bool = False,
    ) -> None:
        half = width / 2.0
        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy

        def contains(px: float, py: float) -> bool:
            if length_sq == 0:
                return (px - x1) * (px - x1) + (py - y1) * (py - y1) <= half * half
            t = ((px - x1) * dx + (py - y1) * dy) / length_sq
            if square_caps:
                if t < 0.0 or t > 1.0:
                    return False
                t = min(1.0, max(0.0, t))
            else:
                t = min(1.0, max(0.0, t))
            cx = x1 + t * dx
            cy = y1 + t * dy
            return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= half * half

        pad = half + 1.0
        self._paint(
            (min(x1, x2) - pad, min(y1, y2) - pad, max(x1, x2) + pad, max(y1, y2) + pad),
            color,
            contains,
        )

    def erase_line(self, x1: float, y1: float, x2: float, y2: float, width: float) -> None:
        half = width / 2.0
        dx = x2 - x1
        dy = y2 - y1
        length_sq = dx * dx + dy * dy

        def contains(px: float, py: float) -> bool:
            if length_sq == 0:
                return (px - x1) * (px - x1) + (py - y1) * (py - y1) <= half * half
            t = ((px - x1) * dx + (py - y1) * dy) / length_sq
            t = min(1.0, max(0.0, t))
            cx = x1 + t * dx
            cy = y1 + t * dy
            return (px - cx) * (px - cx) + (py - cy) * (py - cy) <= half * half

        pad = half + 1.0
        self._paint(
            (min(x1, x2) - pad, min(y1, y2) - pad, max(x1, x2) + pad, max(y1, y2) + pad),
            (0, 0, 0, 255),
            contains,
            erase=True,
        )

    def polyline(self, points: Iterable[tuple[float, float]], width: float, color: tuple[int, int, int, int]) -> None:
        pts = list(points)
        for p1, p2 in zip(pts, pts[1:]):
            self.line(p1[0], p1[1], p2[0], p2[1], width, color)

    def polygon(self, points: Iterable[tuple[float, float]], color: tuple[int, int, int, int]) -> None:
        pts = list(points)
        min_x = min(x for x, _ in pts)
        min_y = min(y for _, y in pts)
        max_x = max(x for x, _ in pts)
        max_y = max(y for _, y in pts)

        def contains(px: float, py: float) -> bool:
            inside = False
            j = len(pts) - 1
            for i, (xi, yi) in enumerate(pts):
                xj, yj = pts[j]
                if ((yi > py) != (yj > py)) and (px < (xj - xi) * (py - yi) / ((yj - yi) or 1e-9) + xi):
                    inside = not inside
                j = i
            return inside

        self._paint((min_x, min_y, max_x, max_y), color, contains)

    def arc(
        self,
        cx: float,
        cy: float,
        radius: float,
        start_deg: float,
        end_deg: float,
        width: float,
        color: tuple[int, int, int, int],
    ) -> None:
        half = width / 2.0
        start = start_deg % 360.0
        end = end_deg % 360.0

        def in_angle(angle: float) -> bool:
            angle %= 360.0
            if start <= end:
                return start <= angle <= end
            return angle >= start or angle <= end

        def contains(px: float, py: float) -> bool:
            dx = px - cx
            dy = py - cy
            dist = math.hypot(dx, dy)
            if abs(dist - radius) > half:
                return False
            angle = math.degrees(math.atan2(dy, dx))
            return in_angle(angle)

        pad = width + 1.0
        self._paint((cx - radius - pad, cy - radius - pad, cx + radius + pad, cy + radius + pad), color, contains)

    def fill_path(self, subpaths: Iterable[list[tuple[float, float]]], color: tuple[int, int, int, int]) -> None:
        """Fill flattened subpaths using the nonzero winding rule (so reverse-wound
        subpaths punch holes, matching SVG's default fill-rule)."""
        edges: list[tuple[float, float, float, float]] = []
        for sp in subpaths:
            if len(sp) < 2:
                continue
            pts = sp if sp[0] == sp[-1] else sp + [sp[0]]
            for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
                if y0 != y1:
                    edges.append((x0, y0, x1, y1))
        if not edges:
            return
        min_x = min(min(e[0], e[2]) for e in edges)
        min_y = min(min(e[1], e[3]) for e in edges)
        max_x = max(max(e[0], e[2]) for e in edges)
        max_y = max(max(e[1], e[3]) for e in edges)

        def contains(px: float, py: float) -> bool:
            wn = 0
            for x0, y0, x1, y1 in edges:
                if y0 <= py:
                    if y1 > py and ((x1 - x0) * (py - y0) - (px - x0) * (y1 - y0)) > 0:
                        wn += 1
                else:
                    if y1 <= py and ((x1 - x0) * (py - y0) - (px - x0) * (y1 - y0)) < 0:
                        wn -= 1
            return wn != 0

        self._paint((min_x, min_y, max_x, max_y), color, contains)

    def stroke_path(self, subpaths: Iterable[list[tuple[float, float]]], width: float, color: tuple[int, int, int, int]) -> None:
        """Stroke flattened subpaths as round-capped polylines (round joins come from
        overlapping caps); single-point subpaths render as a dot."""
        for sp in subpaths:
            if len(sp) == 1:
                self.circle(sp[0][0], sp[0][1], width / 2.0, color)
            elif len(sp) >= 2:
                self.polyline(sp, width, color)

    def png_bytes(self) -> bytes:
        raw = bytearray()
        for y in range(self.height):
            raw.append(0)
            for x in range(self.width):
                idx = y * self.width + x
                a = max(0.0, min(1.0, self.a[idx]))
                if a > 0.0:
                    r = self.r[idx] / a
                    g = self.g[idx] / a
                    b = self.b[idx] / a
                else:
                    r = g = b = 0.0
                raw.extend(
                    (
                        int(max(0, min(255, round(r * 255)))),
                        int(max(0, min(255, round(g * 255)))),
                        int(max(0, min(255, round(b * 255)))),
                        int(max(0, min(255, round(a * 255)))),
                    )
                )
        return write_png_rgba8(self.width, self.height, bytes(raw))

    def png_bytes_over(self, bg: tuple[int, int, int]) -> bytes:
        """Flatten the (premultiplied) canvas over a solid opaque background. Used by
        --preview so white-on-transparent glyphs are actually visible for QA."""
        bgr, bgg, bgb = (c / 255.0 for c in bg)
        raw = bytearray()
        for y in range(self.height):
            raw.append(0)
            for x in range(self.width):
                idx = y * self.width + x
                a = max(0.0, min(1.0, self.a[idx]))
                # self.r/g/b are already premultiplied by alpha.
                r = self.r[idx] + bgr * (1.0 - a)
                g = self.g[idx] + bgg * (1.0 - a)
                b = self.b[idx] + bgb * (1.0 - a)
                raw.extend(
                    (
                        int(max(0, min(255, round(r * 255)))),
                        int(max(0, min(255, round(g * 255)))),
                        int(max(0, min(255, round(b * 255)))),
                        255,
                    )
                )
        return write_png_rgba8(self.width, self.height, bytes(raw))


def write_png_rgba8(width: int, height: int, raw_scanlines: bytes) -> bytes:
    def chunk(kind: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    png = bytearray(b"\x89PNG\r\n\x1a\n")
    png += chunk("IHDR".encode(), struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    png += chunk("IDAT".encode(), zlib.compress(raw_scanlines, 9))
    png += chunk("IEND".encode(), b"")
    return bytes(png)


def draw_battery(c: Canvas, x: float, y: float, color: tuple[int, int, int, int]) -> None:
    c.rounded_rect(x + 0.4, y + 0.4, 15.8, 9.2, 1.4, color)
    c.erase_rect(x + 2.0, y + 2.0, 12.2, 6.0)


# ── Tabler SVG path rendering ────────────────────────────────────────────────
# Semantic glyphs are rasterized from the real Tabler Icons v3.44.0 vector paths
# (MIT — see res/assets/NOTICE.md) instead of hand-tuned primitives, so they match
# Tabler's actual silhouettes. Paths live in a 24×24 viewBox; draw_tabler() fits
# each into its sprite rect.

_TOKEN_RE = re.compile(r"([MmLlHhVvCcSsQqTtAaZz])|([+-]?(?:\d*\.\d+|\d+\.?\d*)(?:[eE][+-]?\d+)?)")

# Outline icons (rendered stroked, Tabler stroke-width 2).
TABLER_OUTLINE = {
    "wifi": (
        "M12 18l.01 0",
        "M9.172 15.172a4 4 0 0 1 5.656 0",
        "M6.343 12.343a8 8 0 0 1 11.314 0",
        "M3.515 9.515c4.686 -4.687 12.284 -4.687 17 0",
    ),
    "wifi-off": (
        "M12 18l.01 0",
        "M9.172 15.172a4 4 0 0 1 5.656 0",
        "M6.343 12.343a7.963 7.963 0 0 1 3.864 -2.14m4.163 .155a7.965 7.965 0 0 1 3.287 2",
        "M3.515 9.515a12 12 0 0 1 3.544 -2.455m3.101 -.92a12 12 0 0 1 10.325 3.374",
        "M3 3l18 18",
    ),
    "bluetooth": ("M7 8l10 8l-5 4l0 -16l5 4l-10 8",),
    "bluetooth-off": (
        "M3 3l18 18",
        "M16.438 16.45l-4.438 3.55v-8m0 -4v-4l5 4l-2.776 2.22m-2.222 1.779l-5 4",
    ),
    "sun": (
        "M8 12a4 4 0 1 0 8 0a4 4 0 1 0 -8 0",
        "M3 12h1m8 -9v1m8 8h1m-9 8v1m-6.4 -15.4l.7 .7m12.1 -.7l-.7 .7m0 11.4l.7 .7m-12.1 -.7l-.7 .7",
    ),
    "temperature": (
        "M10 13.5a4 4 0 1 0 4 0v-8.5a2 2 0 0 0 -4 0v8.5",
        "M10 9l4 0",
    ),
    "settings": (
        "M10.325 4.317c.426 -1.756 2.924 -1.756 3.35 0a1.724 1.724 0 0 0 2.573 1.066c1.543 -.94 3.31 .826 2.37 2.37a1.724 1.724 0 0 0 1.065 2.572c1.756 .426 1.756 2.924 0 3.35a1.724 1.724 0 0 0 -1.066 2.573c.94 1.543 -.826 3.31 -2.37 2.37a1.724 1.724 0 0 0 -2.572 1.065c-.426 1.756 -2.924 1.756 -3.35 0a1.724 1.724 0 0 0 -2.573 -1.066c-1.543 .94 -3.31 -.826 -2.37 -2.37a1.724 1.724 0 0 0 -1.065 -2.572c-1.756 -.426 -1.756 -2.924 0 -3.35a1.724 1.724 0 0 0 1.066 -2.573c-.94 -1.543 .826 -3.31 2.37 -2.37c1 .608 2.296 .07 2.572 -1.065",
        "M9 12a3 3 0 1 0 6 0a3 3 0 0 0 -6 0",
    ),
    "lock": (
        "M5 13a2 2 0 0 1 2 -2h10a2 2 0 0 1 2 2v6a2 2 0 0 1 -2 2h-10a2 2 0 0 1 -2 -2v-6",
        "M11 16a1 1 0 1 0 2 0a1 1 0 0 0 -2 0",
        "M8 11v-4a4 4 0 1 1 8 0v4",
    ),
    "circle-check": (
        "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0",
        "M9 12l2 2l4 -4",
    ),
    "power": (
        "M7 6a7.75 7.75 0 1 0 10 0",
        "M12 4l0 8",
    ),
    "refresh": (
        "M20 11a8.1 8.1 0 0 0 -15.5 -2m-.5 -4v4h4",
        "M4 13a8.1 8.1 0 0 0 15.5 2m.5 4v-4h-4",
    ),
    "moon": (
        "M12 3c.132 0 .263 0 .393 0a7.5 7.5 0 0 0 7.92 12.446a9 9 0 1 1 -8.313 -12.454l0 .008",
    ),
    "cloud-download": (
        "M19 18a3.5 3.5 0 0 0 0 -7h-1a5 4.5 0 0 0 -11 -2a4.6 4.4 0 0 0 -2.1 8.4",
        "M12 13l0 9",
        "M9 19l3 3l3 -3",
    ),
}

# Filled icons (rendered with nonzero-winding fill so cutouts punch holes).
TABLER_FILLED = {
    "headphones": (
        "M21 18a3 3 0 0 1 -2.824 2.995l-.176 .005h-1a3 3 0 0 1 -2.995 -2.824l-.005 -.176v-3a3 3 0 0 1 2.824 -2.995l.176 -.005h1c.351 0 .688 .06 1 .171v-.171a7 7 0 0 0 -13.996 -.24l-.004 .24v.17c.25 -.088 .516 -.144 .791 -.163l.209 -.007h1a3 3 0 0 1 2.995 2.824l.005 .176v3a3 3 0 0 1 -2.824 2.995l-.176 .005h-1a3 3 0 0 1 -2.995 -2.824l-.005 -.176v-6a9 9 0 0 1 17.996 -.265l.004 .265v6z",
    ),
    "device-gamepad": (
        "M20 5a3 3 0 0 1 3 3v8a3 3 0 0 1 -3 3h-16a3 3 0 0 1 -3 -3v-8a3 3 0 0 1 3 -3zm-12 4l-.117 .007a1 1 0 0 0 -.883 .993v1h-1a1 1 0 0 0 -1 1l.007 .117a1 1 0 0 0 .993 .883h1v1a1 1 0 0 0 1 1l.117 -.007a1 1 0 0 0 .883 -.993v-1h1a1 1 0 0 0 1 -1l-.007 -.117a1 1 0 0 0 -.993 -.883h-1v-1a1 1 0 0 0 -1 -1m10 3a1 1 0 0 0 -1 1v.01a1 1 0 0 0 2 0v-.01a1 1 0 0 0 -1 -1m-3 -2a1 1 0 0 0 -1 1v.01a1 1 0 0 0 2 0v-.01a1 1 0 0 0 -1 -1",
    ),
    "device-gamepad-2": (
        "M15.5 4a6 6 0 0 1 5.945 5.187l1.532 7.883a3.3 3.3 0 0 1 -5.632 2.903l-3.776 -3.974l-3.14 .001l-3.719 3.916a3.3 3.3 0 0 1 -5.629 -2.92l1.634 -8.173a6 6 0 0 1 5.885 -4.823zm-7.5 3a1 1 0 0 0 -1 1v1h-1a1 1 0 1 0 0 2h1v1a1 1 0 0 0 2 0v-1h1a1 1 0 0 0 0 -2h-1v-1a1 1 0 0 0 -1 -1m10 2h-4a1 1 0 0 0 0 2h4a1 1 0 0 0 0 -2",
    ),
}

TABLER_STROKE = 2.0


def _cubic(p0, p1, p2, p3, n: int = 16):
    out = []
    for i in range(1, n + 1):
        t = i / n
        mt = 1.0 - t
        a, b, cc, d = mt * mt * mt, 3 * mt * mt * t, 3 * mt * t * t, t * t * t
        out.append((a * p0[0] + b * p1[0] + cc * p2[0] + d * p3[0],
                    a * p0[1] + b * p1[1] + cc * p2[1] + d * p3[1]))
    return out


def _quad(p0, p1, p2, n: int = 14):
    out = []
    for i in range(1, n + 1):
        t = i / n
        mt = 1.0 - t
        a, b, cc = mt * mt, 2 * mt * t, t * t
        out.append((a * p0[0] + b * p1[0] + cc * p2[0],
                    a * p0[1] + b * p1[1] + cc * p2[1]))
    return out


def _arc(p0, rx, ry, rot_deg, large, sweep, p1):
    """Flatten an SVG elliptical arc via endpoint→center parameterization (spec F.6.5)."""
    x0, y0 = p0
    x1, y1 = p1
    rx, ry = abs(rx), abs(ry)
    if rx == 0 or ry == 0 or (x0 == x1 and y0 == y1):
        return [p1]
    phi = math.radians(rot_deg)
    cosp, sinp = math.cos(phi), math.sin(phi)
    dx, dy = (x0 - x1) / 2.0, (y0 - y1) / 2.0
    x1p = cosp * dx + sinp * dy
    y1p = -sinp * dx + cosp * dy
    lam = (x1p * x1p) / (rx * rx) + (y1p * y1p) / (ry * ry)
    if lam > 1.0:
        scl = math.sqrt(lam)
        rx, ry = rx * scl, ry * scl
    num = rx * rx * ry * ry - rx * rx * y1p * y1p - ry * ry * x1p * x1p
    den = rx * rx * y1p * y1p + ry * ry * x1p * x1p
    co = math.sqrt(max(0.0, num / den)) if den != 0 else 0.0
    if (large != 0) == (sweep != 0):
        co = -co
    cxp = co * (rx * y1p / ry)
    cyp = co * (-ry * x1p / rx)
    cx = cosp * cxp - sinp * cyp + (x0 + x1) / 2.0
    cy = sinp * cxp + cosp * cyp + (y0 + y1) / 2.0

    def angle(ux, uy, vx, vy):
        ln = math.hypot(ux, uy) * math.hypot(vx, vy)
        a = math.acos(max(-1.0, min(1.0, (ux * vx + uy * vy) / ln))) if ln else 0.0
        return -a if (ux * vy - uy * vx) < 0 else a

    ux, uy = (x1p - cxp) / rx, (y1p - cyp) / ry
    vx, vy = (-x1p - cxp) / rx, (-y1p - cyp) / ry
    theta1 = angle(1.0, 0.0, ux, uy)
    dtheta = angle(ux, uy, vx, vy)
    if sweep == 0 and dtheta > 0:
        dtheta -= 2 * math.pi
    elif sweep != 0 and dtheta < 0:
        dtheta += 2 * math.pi
    steps = max(2, int(math.ceil(abs(dtheta) / (math.pi / 8.0))))
    out = []
    for i in range(1, steps + 1):
        t = theta1 + dtheta * i / steps
        ct, st = math.cos(t), math.sin(t)
        out.append((cx + rx * ct * cosp - ry * st * sinp,
                    cy + rx * ct * sinp + ry * st * cosp))
    return out


def parse_svg_path(d: str) -> list[list[tuple[float, float]]]:
    """Parse an SVG path 'd' into flattened subpaths (lists of points) in 24×24 space."""
    toks = [("c", m.group(1)) if m.group(1) else ("n", float(m.group(2)))
            for m in _TOKEN_RE.finditer(d)]
    subpaths: list[list[tuple[float, float]]] = []
    cur: list[tuple[float, float]] | None = None
    i, n = 0, len(toks)
    cx = cy = 0.0
    start = (0.0, 0.0)
    prev_cmd = None
    prev_ctrl = None

    def take() -> float:
        nonlocal i
        v = toks[i][1]
        i += 1
        return v

    while i < n:
        if toks[i][0] == "c":
            cmd = toks[i][1]
            i += 1
        else:
            cmd = prev_cmd
            if cmd == "M":
                cmd = "L"
            elif cmd == "m":
                cmd = "l"
        rel = cmd.islower()
        kind = cmd.upper()
        if kind == "Z":
            if cur is not None:
                cur.append(start)
            cx, cy = start
            prev_ctrl = None
            prev_cmd = cmd
            continue
        if kind == "M":
            x, y = take(), take()
            if rel:
                x, y = x + cx, y + cy
            cx, cy = x, y
            start = (cx, cy)
            cur = [(cx, cy)]
            subpaths.append(cur)
            prev_ctrl = None
        elif kind == "L":
            x, y = take(), take()
            if rel:
                x, y = x + cx, y + cy
            cx, cy = x, y
            cur.append((cx, cy))
            prev_ctrl = None
        elif kind == "H":
            x = take()
            cx = x + cx if rel else x
            cur.append((cx, cy))
            prev_ctrl = None
        elif kind == "V":
            y = take()
            cy = y + cy if rel else y
            cur.append((cx, cy))
            prev_ctrl = None
        elif kind == "C":
            x1, y1, x2, y2, x, y = take(), take(), take(), take(), take(), take()
            if rel:
                x1, y1, x2, y2, x, y = x1 + cx, y1 + cy, x2 + cx, y2 + cy, x + cx, y + cy
            cur.extend(_cubic((cx, cy), (x1, y1), (x2, y2), (x, y)))
            prev_ctrl = (x2, y2)
            cx, cy = x, y
        elif kind == "S":
            x2, y2, x, y = take(), take(), take(), take()
            if rel:
                x2, y2, x, y = x2 + cx, y2 + cy, x + cx, y + cy
            if prev_cmd and prev_cmd.upper() in ("C", "S") and prev_ctrl:
                x1, y1 = 2 * cx - prev_ctrl[0], 2 * cy - prev_ctrl[1]
            else:
                x1, y1 = cx, cy
            cur.extend(_cubic((cx, cy), (x1, y1), (x2, y2), (x, y)))
            prev_ctrl = (x2, y2)
            cx, cy = x, y
        elif kind == "Q":
            x1, y1, x, y = take(), take(), take(), take()
            if rel:
                x1, y1, x, y = x1 + cx, y1 + cy, x + cx, y + cy
            cur.extend(_quad((cx, cy), (x1, y1), (x, y)))
            prev_ctrl = (x1, y1)
            cx, cy = x, y
        elif kind == "T":
            x, y = take(), take()
            if rel:
                x, y = x + cx, y + cy
            if prev_cmd and prev_cmd.upper() in ("Q", "T") and prev_ctrl:
                x1, y1 = 2 * cx - prev_ctrl[0], 2 * cy - prev_ctrl[1]
            else:
                x1, y1 = cx, cy
            cur.extend(_quad((cx, cy), (x1, y1), (x, y)))
            prev_ctrl = (x1, y1)
            cx, cy = x, y
        elif kind == "A":
            rx, ry, rot, large, sweep, x, y = (take(), take(), take(), take(), take(), take(), take())
            if rel:
                x, y = x + cx, y + cy
            cur.extend(_arc((cx, cy), rx, ry, rot, large, sweep, (x, y)))
            cx, cy = x, y
            prev_ctrl = None
        prev_cmd = cmd
    return subpaths


def _fit(subpaths, x: float, y: float, w: float, h: float, pad: float):
    """Uniform 'contain' fit of the path's (padded) bbox into the target rect, centered."""
    pts = [p for sp in subpaths for p in sp]
    min_x = min(p[0] for p in pts) - pad
    min_y = min(p[1] for p in pts) - pad
    max_x = max(p[0] for p in pts) + pad
    max_y = max(p[1] for p in pts) + pad
    bw = max(max_x - min_x, 1e-6)
    bh = max(max_y - min_y, 1e-6)
    s = min(w / bw, h / bh)
    tx = x + (w - bw * s) / 2.0 - min_x * s
    ty = y + (h - bh * s) / 2.0 - min_y * s
    return s, tx, ty


def _xform(subpaths, s, tx, ty):
    return [[(px * s + tx, py * s + ty) for px, py in sp] for sp in subpaths]


def draw_tabler(c: Canvas, name: str, x: float, y: float, w: float, h: float,
                color: tuple[int, int, int, int] = WHITE) -> None:
    if name in TABLER_FILLED:
        groups = [parse_svg_path(d) for d in TABLER_FILLED[name]]
        allsp = [sp for g in groups for sp in g]
        s, tx, ty = _fit(allsp, x, y, w, h, 0.0)
        c.fill_path(_xform(allsp, s, tx, ty), color)
    else:
        groups = [parse_svg_path(d) for d in TABLER_OUTLINE[name]]
        allsp = [sp for g in groups for sp in g]
        s, tx, ty = _fit(allsp, x, y, w, h, TABLER_STROKE / 2.0)
        for g in groups:
            c.stroke_path(_xform(g, s, tx, ty), TABLER_STROKE * s, color)


def draw_wifi(c: Canvas, x: float, y: float, w: float, h: float, active: int, off: bool = False) -> None:
    """Tabler wifi, stroked. The dot + 3 arcs share one center, so no overlap; for the
    medium/low sprites we dim arcs above the active signal level."""
    name = "wifi-off" if off else "wifi"
    groups = [parse_svg_path(d) for d in TABLER_OUTLINE[name]]
    allsp = [sp for g in groups for sp in g]
    s, tx, ty = _fit(allsp, x, y, w, h, TABLER_STROKE / 2.0)
    sw = TABLER_STROKE * s
    # Subpath groups: [dot, inner arc, mid arc, outer arc] -> signal levels [1, 1, 2, 3].
    levels = (1, 1, 2, 3)
    for idx, g in enumerate(groups):
        if off:
            col = WHITE
        else:
            level = levels[idx] if idx < len(levels) else 3
            col = WHITE if level <= active else LIGHT_GRAY
        c.stroke_path(_xform(g, s, tx, ty), sw, col)


def draw_wifi_small(c: Canvas, x: float, y: float) -> None:
    cx = x + 6.0
    cy = y + 8.0
    c.arc(cx, cy, 4.8, 224, 316, 0.95, WHITE)
    c.arc(cx, cy, 2.9, 225, 315, 0.95, WHITE)
    c.arc(cx, cy, 1.3, 226, 314, 0.95, WHITE)


def draw_volume(c: Canvas, x: float, y: float, w: float, h: float, waves: bool) -> None:
    c.polygon([(x + 0.8, y + 7.0), (x + 3.0, y + 7.0), (x + 6.0, y + 4.0), (x + 6.0, y + 15.0), (x + 3.0, y + 12.0), (x + 0.8, y + 12.0)], WHITE)
    if waves:
        c.arc(x + 5.8, y + 9.5, 4.6, 310, 50, 1.25, WHITE)
        c.arc(x + 5.8, y + 9.5, 7.6, 315, 45, 1.25, WHITE)


def render(canvas: Canvas) -> None:
    # Geometric primitives and color swatches.
    canvas.circle(16, 16, 15, WHITE)
    canvas.circle(48, 16, 15, BLACK)
    canvas.circle(80, 16, 15, DARK_GRAY)
    canvas.circle(107, 11, 10, DARK_GRAY)
    canvas.circle(11, 43, 10, WHITE)
    canvas.circle(71.5, 40.5, 7.5, WHITE)
    canvas.circle(27, 58, 4, WHITE)
    canvas.circle(42, 57, 3, BLACK)
    canvas.rounded_rect(33, 58, 4, 4, 1, WHITE)
    canvas.circle(34, 55, 1, MID_GRAY)
    canvas.circle(86.5, 52.5, 1.5, MID_GRAY)
    canvas.circle(11, 73, 10, BLACK)
    canvas.circle(17, 57, 2, BLACK)
    canvas.circle(87, 58, 2, DARK_GRAY)

    # Scroll chevrons.
    canvas.polygon([(97, 28), (109, 23), (121, 28), (121, 29), (97, 29)], DARK_GRAY)
    canvas.polygon([(97, 31), (121, 31), (121, 32), (109, 37), (97, 32)], DARK_GRAY)

    # Battery pieces.
    draw_battery(canvas, 47, 51, WHITE)
    draw_battery(canvas, 66, 51, RED)
    canvas.rect(81, 33, 12, 6, WHITE)
    canvas.rect(1, 55, 12, 6, RED)
    canvas.polygon([(84, 41), (90, 41), (87, 43.1), (91, 43.1), (84, 47), (86.2, 44.4), (82, 44.4)], WHITE)

    # Hardware and status glyphs (Tabler-derived; draw_volume kept procedural because
    # ASSET_VOLUME_MUTE samples the left sub-rect of the same drawing).
    draw_tabler(canvas, "sun", 1, 85, 19, 19)
    draw_volume(canvas, 21, 85, 19, 19, waves=True)
    draw_tabler(canvas, "temperature", 41, 85, 9, 19)
    draw_wifi(canvas, 1, 104, 12, 12, active=3)
    draw_wifi(canvas, 14, 104, 12, 12, active=2)
    draw_wifi(canvas, 27, 104, 12, 12, active=1)
    draw_wifi(canvas, 40, 104, 12, 12, active=3, off=True)
    draw_tabler(canvas, "bluetooth", 53, 104, 12, 12)
    draw_tabler(canvas, "bluetooth-off", 66, 104, 12, 12)
    draw_tabler(canvas, "headphones", 79, 104, 12, 12)
    draw_tabler(canvas, "device-gamepad-2", 92, 104, 12, 12)

    # Footer/settings glyphs.
    draw_tabler(canvas, "circle-check", 1, 117, 10, 10)
    draw_tabler(canvas, "lock", 12, 116, 8, 11)
    draw_tabler(canvas, "settings", 21, 117, 10, 10)
    draw_tabler(canvas, "moon", 32, 117, 10, 10)
    draw_tabler(canvas, "power", 43, 117, 10, 10)
    draw_tabler(canvas, "refresh", 54, 119, 11, 8)
    draw_tabler(canvas, "cloud-download", 66, 117, 10, 10)
    draw_tabler(canvas, "device-gamepad", 91, 51, 17, 10)

    # Legacy unmapped clusters kept for future compatibility/debug parity.
    draw_wifi_small(canvas, 96, 40)


def write_manifest(path: Path) -> None:
    rows = ["# Generated Asset Atlas Manifest", "", "| Asset | 1x rect | Source |", "| --- | ---: | --- |"]
    for asset in ASSET_MANIFEST:
        x, y, w, h = asset.rect
        rows.append(f"| `{asset.name}` | `{int(x)},{int(y)},{int(w)},{int(h)}` | {asset.source} |")
    path.write_text("\n".join(rows) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", type=Path, default=Path("res/assets"), help="directory for generated assets")
    parser.add_argument("--manifest", action="store_true", help="also write MANIFEST.md")
    parser.add_argument("--preview", action="store_true",
                        help="also write preview.png (4x atlas over a dark background for QA)")
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    preview_canvas = None
    for scale in SCALES:
        canvas = Canvas(scale)
        render(canvas)
        (args.out_dir / f"assets@{scale}x.png").write_bytes(canvas.png_bytes())
        if args.preview and scale == 4:
            preview_canvas = canvas
    if args.manifest:
        write_manifest(args.out_dir / "MANIFEST.md")
    if args.preview and preview_canvas is not None:
        (args.out_dir / "preview.png").write_bytes(preview_canvas.png_bytes_over((30, 30, 30)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
