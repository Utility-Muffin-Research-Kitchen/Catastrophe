#!/usr/bin/env python3
"""Generate Catastrophe's status/control asset atlas.

The original development preview atlas came from NextUI. This generator keeps
the same 1x sprite coordinates, but redraws the sprites from simple procedural
geometry and Tabler-icon-derived glyphs so the generated PNGs are redistributable.
"""

from __future__ import annotations

import argparse
import math
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
    Asset("ASSET_AUDIO", (79, 104, 12, 12), "Tabler headphones"),
    Asset("ASSET_CONTROLLER", (92, 104, 12, 12), "Tabler device-gamepad-2"),
    Asset("ASSET_CHECKCIRCLE", (1, 117, 10, 10), "Tabler circle-check"),
    Asset("ASSET_LOCK", (12, 116, 8, 11), "Tabler lock"),
    Asset("ASSET_SETTINGS", (21, 117, 10, 10), "Tabler settings"),
    Asset("ASSET_STORE", (66, 117, 10, 10), "Tabler building-store"),
    Asset("ASSET_GAMEPAD", (91, 51, 17, 10), "Tabler device-gamepad-2"),
    Asset("ASSET_POWEROFF", (43, 117, 10, 10), "Tabler power"),
    Asset("ASSET_RESTART", (54, 119, 11, 8), "Tabler refresh"),
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


def draw_wifi(c: Canvas, x: float, y: float, active: int, off: bool = False) -> None:
    if off:
        color_for = lambda _level: WHITE
    else:
        color_for = lambda level: WHITE if level <= active else LIGHT_GRAY
    cx = x + 6.0
    cy = y + 12.0
    c.arc(cx, cy, 6.35, 224, 316, 1.2, color_for(3))
    c.arc(cx, cy, 4.15, 225, 315, 1.2, color_for(2))
    c.arc(cx, cy, 2.2, 226, 314, 1.2, color_for(1))
    c.circle(cx, y + 9.2, 0.75, color_for(1))
    if off:
        c.line(x + 1.7, y + 2.2, x + 10.2, y + 10.6, 1.15, WHITE)


def draw_wifi_small(c: Canvas, x: float, y: float) -> None:
    cx = x + 6.0
    cy = y + 8.0
    c.arc(cx, cy, 4.8, 224, 316, 0.95, WHITE)
    c.arc(cx, cy, 2.9, 225, 315, 0.95, WHITE)
    c.arc(cx, cy, 1.3, 226, 314, 0.95, WHITE)


def draw_bluetooth(c: Canvas, x: float, y: float, off: bool = False) -> None:
    pts = [(x + 5.2, y + 1.4), (x + 8.8, y + 4.6), (x + 5.2, y + 7.9), (x + 5.2, y + 1.4), (x + 5.2, y + 10.6), (x + 8.8, y + 7.4), (x + 5.2, y + 4.1)]
    c.polyline(pts, 1.25, WHITE)
    c.line(x + 2.5, y + 3.2, x + 5.2, y + 6.0, 1.25, WHITE)
    c.line(x + 2.5, y + 8.8, x + 5.2, y + 6.0, 1.25, WHITE)
    if off:
        c.line(x + 1.6, y + 1.3, x + 10.6, y + 10.3, 1.3, WHITE)


def draw_audio(c: Canvas, x: float, y: float) -> None:
    c.arc(x + 6.0, y + 6.8, 4.7, 200, 340, 1.35, WHITE)
    c.line(x + 1.8, y + 6.9, x + 1.8, y + 9.8, 1.6, WHITE)
    c.line(x + 10.2, y + 6.9, x + 10.2, y + 9.8, 1.6, WHITE)
    c.line(x + 2.0, y + 9.6, x + 4.0, y + 10.6, 1.15, WHITE)
    c.line(x + 10.0, y + 9.6, x + 8.0, y + 10.6, 1.15, WHITE)


def draw_gamepad(c: Canvas, x: float, y: float, w: float, h: float) -> None:
    c.rounded_rect(x + 0.8, y + 2.0, w - 1.6, h - 2.8, 2.6, WHITE)
    c.erase_circle(x + 4.0, y + 5.0, 0.75)
    c.erase_circle(x + w - 4.0, y + 4.8, 0.65)
    c.erase_circle(x + w - 2.6, y + 6.3, 0.65)
    c.erase_line(x + 5.6, y + 4.8, x + 7.8, y + 4.8, 0.85)


def draw_sun(c: Canvas, x: float, y: float, w: float, h: float) -> None:
    cx, cy = x + w / 2.0, y + h / 2.0
    c.circle(cx, cy, min(w, h) * 0.23, WHITE)
    for i in range(8):
        a = math.tau * i / 8.0
        r1 = min(w, h) * 0.36
        r2 = min(w, h) * 0.42
        c.line(cx + math.cos(a) * r1, cy + math.sin(a) * r1, cx + math.cos(a) * r2, cy + math.sin(a) * r2, 1.2, WHITE)


def draw_volume(c: Canvas, x: float, y: float, w: float, h: float, waves: bool) -> None:
    c.polygon([(x + 0.8, y + 7.0), (x + 3.0, y + 7.0), (x + 6.0, y + 4.0), (x + 6.0, y + 15.0), (x + 3.0, y + 12.0), (x + 0.8, y + 12.0)], WHITE)
    if waves:
        c.arc(x + 5.8, y + 9.5, 4.6, 310, 50, 1.25, WHITE)
        c.arc(x + 5.8, y + 9.5, 7.6, 315, 45, 1.25, WHITE)


def draw_temperature(c: Canvas, x: float, y: float) -> None:
    c.line(x + 4.5, y + 3.0, x + 4.5, y + 12.0, 1.8, WHITE)
    c.circle(x + 4.5, y + 14.2, 2.3, WHITE)
    c.line(x + 4.5, y + 3.0, x + 4.5, y + 3.0, 3.0, WHITE)


def draw_checkcircle(c: Canvas, x: float, y: float) -> None:
    c.circle(x + 5.0, y + 5.0, 4.7, WHITE)
    c.erase_circle(x + 5.0, y + 5.0, 3.2)
    c.polyline([(x + 2.9, y + 5.1), (x + 4.3, y + 6.5), (x + 7.3, y + 3.6)], 1.2, WHITE)


def draw_lock(c: Canvas, x: float, y: float) -> None:
    c.rounded_rect(x + 0.8, y + 5.0, 6.4, 5.3, 1.0, WHITE)
    c.arc(x + 4.0, y + 5.6, 3.0, 200, 340, 1.2, WHITE)


def draw_settings(c: Canvas, x: float, y: float) -> None:
    cx, cy = x + 5.0, y + 5.0
    for i in range(8):
        a = math.tau * i / 8.0
        c.line(cx + math.cos(a) * 2.15, cy + math.sin(a) * 2.15, cx + math.cos(a) * 3.8, cy + math.sin(a) * 3.8, 1.05, WHITE)
    c.circle(cx, cy, 3.0, WHITE)
    c.erase_circle(cx, cy, 1.25)


def draw_store(c: Canvas, x: float, y: float) -> None:
    c.polygon([(x + 1.2, y + 4.2), (x + 2.2, y + 1.2), (x + 7.8, y + 1.2), (x + 8.8, y + 4.2)], WHITE)
    c.rect(x + 1.6, y + 4.0, 6.8, 1.1, WHITE)
    c.rounded_rect(x + 2.0, y + 5.0, 6.0, 4.0, 0.7, WHITE)
    c.erase_rect(x + 3.0, y + 6.0, 4.0, 3.0)


def draw_power(c: Canvas, x: float, y: float) -> None:
    c.arc(x + 5.0, y + 5.7, 4.0, 125, 415, 1.35, WHITE)
    c.line(x + 5.0, y + 0.8, x + 5.0, y + 5.2, 1.35, WHITE)


def draw_refresh(c: Canvas, x: float, y: float) -> None:
    c.arc(x + 5.7, y + 4.0, 2.85, 145, 430, 1.05, WHITE)
    c.polygon([(x + 8.0, y + 1.4), (x + 10.0, y + 2.2), (x + 8.3, y + 3.4)], WHITE)


def draw_moon(c: Canvas, x: float, y: float) -> None:
    c.circle(x + 5.3, y + 5.0, 4.4, WHITE)
    c.erase_circle(x + 7.2, y + 3.8, 4.2)


def draw_controller(c: Canvas, x: float, y: float) -> None:
    draw_gamepad(c, x + 0.5, y + 1.0, 11.0, 9.5)


def draw_headphones(c: Canvas, x: float, y: float) -> None:
    draw_audio(c, x, y)


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

    # Hardware and status glyphs.
    draw_sun(canvas, 1, 85, 19, 19)
    draw_volume(canvas, 21, 85, 19, 19, waves=True)
    draw_temperature(canvas, 41, 85)
    draw_wifi(canvas, 1, 104, active=3)
    draw_wifi(canvas, 14, 104, active=2)
    draw_wifi(canvas, 27, 104, active=1)
    draw_wifi(canvas, 40, 104, active=3, off=True)
    draw_bluetooth(canvas, 53, 104)
    draw_bluetooth(canvas, 66, 104, off=True)
    draw_headphones(canvas, 79, 104)
    draw_controller(canvas, 92, 104)

    # Footer/settings glyphs.
    draw_checkcircle(canvas, 1, 117)
    draw_lock(canvas, 12, 116)
    draw_settings(canvas, 21, 117)
    draw_moon(canvas, 32, 117)
    draw_power(canvas, 43, 117)
    draw_refresh(canvas, 54, 119)
    draw_store(canvas, 66, 117)
    draw_gamepad(canvas, 91, 51, 17, 10)

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
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    for scale in SCALES:
        canvas = Canvas(scale)
        render(canvas)
        (args.out_dir / f"assets@{scale}x.png").write_bytes(canvas.png_bytes())
    if args.manifest:
        write_manifest(args.out_dir / "MANIFEST.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
