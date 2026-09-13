#!/usr/bin/env python3
"""Convert the Qevoryx logo into compact ncurses-ready block art.

The generated preview uses terminal cells twice as tall as they are wide.
Unicode half blocks represent two source pixels in one terminal cell.
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image


@dataclass(frozen=True)
class Pixel:
    red: int
    green: int
    blue: int
    alpha: int

    @property
    def is_transparent(self) -> bool:
        return self.alpha == 0

    @property
    def rgb(self) -> tuple[int, int, int]:
        return self.red, self.green, self.blue


@dataclass(frozen=True)
class Cell:
    glyph: str
    foreground: Pixel | None
    background: Pixel | None


BASE_COLORS: dict[str, tuple[int, int, int]] = {
    "black": (0, 0, 0),
    "red": (205, 49, 49),
    "green": (13, 188, 121),
    "yellow": (229, 193, 0),
    "blue": (36, 114, 200),
    "magenta": (188, 63, 188),
    "cyan": (17, 168, 205),
    "white": (249, 241, 229),
}

# --- Brand mode -------------------------------------------------------------
# The Qevoryx logo is a white dragon/wolf + "Q" with red accents on a
# transparent/black field. For a terminal we quantise every pixel to one of
# three "tones": Light (the white body), Red (the accents), or None. Both the
# black outlines and the transparent pixels map to None, so the terminal
# background shows through the gaps -- this is what keeps the shape readable
# instead of turning into a black-on-black smear.

# Preview swatches; the C++ side owns the real terminal colors.
BRAND_TONE_RGB: dict[str, tuple[int, int, int]] = {
    "L": (243, 243, 246),
    "D": (132, 142, 158),
    "R": (230, 32, 42),
}

# A terminal cell is roughly twice as tall as it is wide. Each cell holds two
# stacked half-block pixels, so one cell renders as two screen-square pixels
# vertically. To reproduce the source without vertical stretch, a `width`-cell
# render must be `width * aspect` pixels tall and therefore `width * aspect / 2`
# cells tall -- for the square logo that is width/2 rows, NOT width rows.

# Splash logo widths (largest that fits the terminal is used at runtime) and the
# header emblem widths (largest that fits the header interior is used). Both are
# rendered as true squares.
BRAND_SPLASH_WIDTHS = (56, 44, 32, 22)
BRAND_EMBLEM_WIDTHS = (16, 14, 12, 10, 8)
FTXUI_EMBLEM_WIDTHS = (24, 20, 16, 12, 8)
FTXUI_INTRO_WIDTH = 96
FTXUI_INTRO_HEIGHT = 27
FTXUI_INTRO_FRAMES = 36


def classify_brand(pixel: Pixel, alpha_threshold: int) -> str | None:
    if pixel.alpha < alpha_threshold:
        return None
    red, green, blue = pixel.rgb
    if red > 110 and red > green * 1.7 and red > blue * 1.7:
        return "R"
    luminance = 0.299 * red + 0.587 * green + 0.114 * blue
    if luminance > 120:
        return "L"
    if luminance >= 60:
        return "D"
    return None


def brand_rows(
    image_path: Path, width: int, alpha_threshold: int
) -> list[list[tuple[str | None, str | None]]]:
    """Return rows of (top_tone, bottom_tone) pairs, one pair per terminal cell.

    The result is undistorted: a square source produces a `width` x `width/2`
    cell grid, which renders as a visual square because each cell is ~twice as
    tall as it is wide and holds two stacked half-block pixels.
    """
    image = Image.open(image_path).convert("RGBA")
    mask = image.getchannel("A").point(lambda v: 255 if v >= alpha_threshold else 0)
    bounds = mask.getbbox()
    if bounds is not None:
        image = image.crop(bounds)

    # Sample to `width` px wide and a matching number of px tall for the source
    # aspect ratio; two stacked px per cell => the cell grid keeps proportions.
    pixel_height = max(2, round(image.height / image.width * width))
    if pixel_height % 2:
        pixel_height += 1
    image = image.resize((width, pixel_height), Image.Resampling.LANCZOS)

    flat = flattened_pixels(image)
    grid = [
        [Pixel(*flat[y * width + x]) for x in range(width)]
        for y in range(pixel_height)
    ]

    rows: list[list[tuple[str | None, str | None]]] = []
    for y in range(0, pixel_height, 2):
        row: list[tuple[str | None, str | None]] = []
        for x in range(width):
            top = classify_brand(grid[y][x], alpha_threshold)
            bottom = (
                classify_brand(grid[y + 1][x], alpha_threshold)
                if y + 1 < pixel_height
                else None
            )
            row.append((top, bottom))
        rows.append(row)
    return rows


def brand_cell(top: str | None, bottom: str | None) -> tuple[int, str, str]:
    """Resolve a cell to (glyph_code, fg_tone, bg_tone).

    glyph_code: 0=space 1=upper-half 2=lower-half 3=full block.
    """
    if top and bottom:
        if top == bottom:
            return 3, top, "N"        # full block, single color
        return 1, top, bottom          # upper half fg=top over bg=bottom
    if top:
        return 1, top, "N"             # upper half
    if bottom:
        return 2, bottom, "N"          # lower half
    return 0, "N", "N"                 # empty


def emit_brand_art(name: str, rows: list[list[tuple[str | None, str | None]]]) -> str:
    height = len(rows)
    width = len(rows[0]) if rows else 0
    lines = [f"inline constexpr Cell {name}_cells[] = {{"]
    for row in rows:
        cells = ", ".join(
            "{%d, %s, %s}" % brand_cell(top, bottom) for top, bottom in row
        )
        lines.append(f"    {cells},")
    lines.append("};")
    lines.append(
        f"inline constexpr Art {name}{{{width}, {height}, {name}_cells}};"
    )
    return "\n".join(lines)


def emit_brand_header(image_path: Path) -> str:
    parts = [
        "#pragma once",
        "",
        "// Generated by tools/logo_converter.py --brand-header.",
        "// Source: assets/logo.png. Do not edit by hand; regenerate instead.",
        "//",
        "// Cell::glyph 0=' ' 1=upper-half 2=lower-half 3=full block.",
        "// Tone N=none/transparent L=light(white body) R=red(accent).",
        "// Every Art is a true square (width x width/2 cells) so it renders",
        "// undistorted on a terminal whose cells are ~twice as tall as wide.",
        "",
        '#include "ui/logo.hpp"',
        "",
        "namespace ui {",
        "namespace logo {",
        "",
    ]
    for width in BRAND_SPLASH_WIDTHS:
        rows = brand_rows(image_path, width, alpha_threshold=40)
        parts.append(emit_brand_art(f"Splash{width}", rows))
        parts.append("")
    parts.append("// Splash logos, largest first; runtime picks the largest that fits.")
    parts.append("inline constexpr Art kSplashSizes[] = {")
    parts.append("    " + ", ".join(f"Splash{w}" for w in BRAND_SPLASH_WIDTHS) + ",")
    parts.append("};")
    parts.append("")
    for width in BRAND_EMBLEM_WIDTHS:
        rows = brand_rows(image_path, width, alpha_threshold=40)
        parts.append(emit_brand_art(f"Emblem{width}", rows))
        parts.append("")
    parts.append("// Header emblems, largest first; the header picks the largest that fits.")
    parts.append("inline constexpr Art kEmblemSizes[] = {")
    parts.append("    " + ", ".join(f"Emblem{w}" for w in BRAND_EMBLEM_WIDTHS) + ",")
    parts.append("};")
    parts.append("")
    parts.append("} // namespace logo")
    parts.append("} // namespace ui")
    parts.append("")
    return "\n".join(parts)


def ftxui_cell(top: str | None, bottom: str | None) -> tuple[str, str, str]:
    if top and bottom:
        if top == bottom:
            return '█', top, 'N'
        return '▀', top, bottom
    if top:
        return '▀', top, 'N'
    if bottom:
        return '▄', bottom, 'N'
    return ' ', 'N', 'N'


def emit_ftxui_emblem_art(
    name: str, rows: list[list[tuple[str | None, str | None]]]
) -> str:
    lines = [f'inline const std::vector<std::vector<EmblemCell>> {name} = {{']
    for row in rows:
        cells = ', '.join(
            '{"%s",%s,%s}' % ftxui_cell(top, bottom) for top, bottom in row
        )
        lines.append(f'  {{{cells}}},')
    lines.append('};')
    return '\n'.join(lines)


def emit_ftxui_logo_data(image_path: Path) -> str:
    parts = [
        '#pragma once',
        '#include <string>',
        '#include <vector>',
        '// Generated by tools/logo_converter.py --ftxui-logo-data.',
        '// Source: assets/logo.png. Half-block emblem with light, shadow, and red tones.',
        'namespace logo {',
        'enum Tone { N, L, D, R };',
        'struct EmblemCell { std::string glyph; Tone fg; Tone bg; };',
        '',
    ]
    for width in FTXUI_EMBLEM_WIDTHS:
        rows = brand_rows(image_path, width, alpha_threshold=40)
        parts.append(f'// Emblem{width}: {width}x{len(rows)} cells (aspect-preserving)')
        parts.append(emit_ftxui_emblem_art(f'Emblem{width}', rows))
        parts.append('')
    parts.extend(['} // namespace logo', ''])
    return '\n'.join(parts)


def intro_rows(
    image_path: Path,
    frame_index: int,
    width: int,
    height: int,
    alpha_threshold: int,
) -> list[list[tuple[str | None, str | None]]]:
    image = Image.open(image_path)
    image.seek(frame_index)
    frame = image.convert('RGBA').resize(
        (width, height * 2), Image.Resampling.LANCZOS
    )
    flat = flattened_pixels(frame)
    rows: list[list[tuple[str | None, str | None]]] = []
    for y in range(0, height * 2, 2):
        row: list[tuple[str | None, str | None]] = []
        for x in range(width):
            top = classify_brand(Pixel(*flat[y * width + x]), alpha_threshold)
            bottom = classify_brand(
                Pixel(*flat[(y + 1) * width + x]), alpha_threshold
            )
            row.append((top, bottom))
        rows.append(row)
    return rows


def emit_ftxui_intro_data(image_path: Path) -> str:
    image = Image.open(image_path)
    frame_count = getattr(image, 'n_frames', 1)
    step = max(1, frame_count // FTXUI_INTRO_FRAMES)
    selected_frames = list(range(0, frame_count, step))[:FTXUI_INTRO_FRAMES]

    parts = [
        '#pragma once',
        '#include <string>',
        '#include <vector>',
        '#include "logo_data.hpp"',
        '// Generated by tools/logo_converter.py --ftxui-intro-data.',
        '// Source: assets/intro.gif. Half-block animation frames.',
        'namespace logo {',
        '',
    ]
    frame_names = []
    for index, frame_index in enumerate(selected_frames):
        name = f'IntroFrame{index}'
        frame_names.append(name)
        rows = intro_rows(
            image_path,
            frame_index,
            FTXUI_INTRO_WIDTH,
            FTXUI_INTRO_HEIGHT,
            alpha_threshold=40,
        )
        parts.append(
            f'// {name}: source frame {frame_index}, '
            f'{FTXUI_INTRO_WIDTH}x{FTXUI_INTRO_HEIGHT} cells'
        )
        parts.append(emit_ftxui_emblem_art(name, rows))
        parts.append('')

    parts.append(
        'inline const std::vector<std::vector<std::vector<EmblemCell>>> '
        'IntroFrames = {'
    )
    parts.extend(f'  {name},' for name in frame_names)
    parts.append('};')
    parts.extend(['} // namespace logo', ''])
    return '\n'.join(parts)


def render_brand_preview(
    rows: list[list[tuple[str | None, str | None]]], out_path: Path,
    cell_w: int = 12, cell_h: int = 24,
) -> None:
    from PIL import ImageDraw

    background = (16, 16, 20)
    width = len(rows[0]) if rows else 0
    height = len(rows)
    image = Image.new("RGB", (width * cell_w, height * cell_h), background)
    draw = ImageDraw.Draw(image)
    for y, row in enumerate(rows):
        for x, (top, bottom) in enumerate(row):
            px, py = x * cell_w, y * cell_h
            if top:
                draw.rectangle([px, py, px + cell_w - 1, py + cell_h // 2 - 1],
                               fill=BRAND_TONE_RGB[top])
            if bottom:
                draw.rectangle([px, py + cell_h // 2, px + cell_w - 1, py + cell_h - 1],
                               fill=BRAND_TONE_RGB[bottom])
    out_path.parent.mkdir(parents=True, exist_ok=True)
    image.save(out_path)


def flattened_pixels(image: Image.Image) -> list[tuple[int, int, int, int]]:
    if hasattr(image, "get_flattened_data"):
        return list(image.get_flattened_data())
    return list(image.getdata())


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", nargs="?", default="assets/logo.png")
    parser.add_argument("--width", type=int, default=6, help="terminal-cell width")
    parser.add_argument("--height", type=int, help="terminal-cell height")
    parser.add_argument("--alpha-threshold", type=int, default=32)
    parser.add_argument(
        "--mode",
        choices=("shape", "color"),
        default="shape",
        help="render one selected color or preserve approximate terminal colors",
    )
    parser.add_argument(
        "--shape-color",
        default="cyan",
        choices=tuple(BASE_COLORS),
        help="color used in shape mode",
    )
    parser.add_argument("--output", type=Path, help="write generated C++ rows to this file")
    parser.add_argument("--json-output", type=Path, help="write sampled cells as JSON")
    parser.add_argument("--no-preview", action="store_true")
    parser.add_argument(
        "--brand-header",
        type=Path,
        help="write the brand-tone C++ logo data header (splash sizes + emblem) and exit",
    )
    parser.add_argument(
        "--brand-preview",
        type=Path,
        help="write PNG previews of the brand-tone splash sizes and emblem into this dir",
    )
    parser.add_argument(
        '--ftxui-logo-data',
        type=Path,
        help='write the responsive FTXUI emblem data header and exit',
    )
    parser.add_argument(
        '--ftxui-intro-data',
        type=Path,
        help='write the animated FTXUI intro data header and exit',
    )
    return parser.parse_args()


def load_pixels(path: Path, alpha_threshold: int) -> tuple[list[list[Pixel]], int, int]:
    image = Image.open(path).convert("RGBA")
    alpha = image.getchannel("A").point(
        lambda value: 255 if value >= alpha_threshold else 0
    )
    bounds = alpha.getbbox()
    if bounds is not None:
        image = image.crop(bounds)

    flat_pixels = flattened_pixels(image)
    pixels = [
        [Pixel(*flat_pixels[y * image.width + x]) for x in range(image.width)]
        for y in range(image.height)
    ]
    return pixels, image.height, image.width


def nearest_color(pixel: Pixel) -> str:
    return min(
        BASE_COLORS,
        key=lambda name: sum(
            (channel - wanted) ** 2
            for channel, wanted in zip(BASE_COLORS[name], pixel.rgb)
        ),
    )


def resize_pixels(
    pixels: list[list[Pixel]],
    source_width: int,
    source_height: int,
    target_width: int,
    target_height: int,
) -> list[list[Pixel]]:
    source = Image.new("RGBA", (source_width, source_height))
    source.putdata([(*pixel.rgb, pixel.alpha) for row in pixels for pixel in row])
    target = source.resize((target_width, target_height), Image.Resampling.LANCZOS)
    flat_pixels = flattened_pixels(target)
    return [
        [Pixel(*flat_pixels[y * target_width + x]) for x in range(target_width)]
        for y in range(target_height)
    ]


def make_cell(top: Pixel, bottom: Pixel, mode: str, shape_color: str) -> Cell:
    if mode == "shape":
        color = Pixel(*BASE_COLORS[shape_color], 255)
        if top.is_transparent and bottom.is_transparent:
            return Cell(" ", None, None)
        if bottom.is_transparent:
            return Cell("▀", color, None)
        if top.is_transparent:
            return Cell("▄", color, None)
        return Cell("█", color, None)

    if top.is_transparent and bottom.is_transparent:
        return Cell(" ", None, None)
    if bottom.is_transparent:
        return Cell("▀", top, None)
    if top.is_transparent:
        return Cell("▄", bottom, None)
    if nearest_color(top) == nearest_color(bottom):
        return Cell("█", top, None)
    return Cell("▀", top, bottom)


def build_cells(args: argparse.Namespace) -> list[list[Cell]]:
    pixels, source_height, source_width = load_pixels(
        Path(args.image), args.alpha_threshold
    )
    if args.width <= 0:
        raise ValueError("--width must be positive")

    if args.height is None:
        pixel_height = max(
            1,
            round(source_height / source_width * args.width * 2),
        )
    else:
        if args.height <= 0:
            raise ValueError("--height must be positive")
        pixel_height = args.height * 2

    target_width = args.width
    sampled = resize_pixels(
        pixels,
        source_width,
        source_height,
        target_width,
        pixel_height,
    )

    cells: list[list[Cell]] = []
    for y in range(0, pixel_height, 2):
        row: list[Cell] = []
        for x in range(target_width):
            top = sampled[y][x]
            bottom = sampled[y + 1][x] if y + 1 < pixel_height else Pixel(0, 0, 0, 0)
            row.append(make_cell(top, bottom, args.mode, args.shape_color))
        cells.append(row)
    return cells


def ansi_color(pixel: Pixel | None, background: bool = False) -> str:
    if pixel is None:
        return "49" if background else "39"
    prefix = "48" if background else "38"
    return f"{prefix};2;{pixel.red};{pixel.green};{pixel.blue}"


def print_preview(cells: list[list[Cell]]) -> None:
    for row in cells:
        line = "\033[0m"
        for cell in row:
            if cell.glyph == " ":
                line += " "
                continue
            line += (
                f"\033[{ansi_color(cell.foreground)};"
                f"{ansi_color(cell.background, background=True)}m{cell.glyph}"
            )
        print(line + "\033[0m")


def cpp_rows(cells: list[list[Cell]]) -> str:
    lines = [f"const std::array<std::string, {len(cells)}> logo_rows = {{"]
    for row in cells:
        glyphs = "".join(cell.glyph for cell in row)
        escaped = "".join(
            character if ord(character) < 128 else f"\\u{ord(character):04x}"
            for character in glyphs
        )
        lines.append(f'    u8"{escaped}",')
    lines.append("};")
    return "\n".join(lines) + "\n"


def json_cells(cells: list[list[Cell]]) -> str:
    data = {
        "width": len(cells[0]),
        "height": len(cells),
        "rows": [
            [
                {
                    "glyph": cell.glyph,
                    "foreground": None if cell.foreground is None else cell.foreground.rgb,
                    "background": None if cell.background is None else cell.background.rgb,
                }
                for cell in row
            ]
            for row in cells
        ],
    }
    return json.dumps(data, indent=2) + "\n"


def main() -> None:
    if sys.stdout.encoding and sys.stdout.encoding.lower() not in {"utf-8", "utf8"}:
        sys.stdout.reconfigure(encoding="utf-8")
    args = parse_args()

    if args.brand_header is not None:
        args.brand_header.parent.mkdir(parents=True, exist_ok=True)
        args.brand_header.write_text(emit_brand_header(Path(args.image)), encoding="utf-8")
        print(f"wrote {args.brand_header}")

    if args.brand_preview is not None:
        for width in BRAND_SPLASH_WIDTHS:
            render_brand_preview(
                brand_rows(Path(args.image), width, alpha_threshold=40),
                args.brand_preview / f"splash-{width}.png",
            )
        for width in BRAND_EMBLEM_WIDTHS:
            render_brand_preview(
                brand_rows(Path(args.image), width, alpha_threshold=40),
                args.brand_preview / f"emblem-{width}.png",
            )
        print(f"wrote previews to {args.brand_preview}")

    if args.brand_header is not None or args.brand_preview is not None:
        return

    if args.ftxui_logo_data is not None:
        args.ftxui_logo_data.parent.mkdir(parents=True, exist_ok=True)
        args.ftxui_logo_data.write_text(
            emit_ftxui_logo_data(Path(args.image)), encoding='utf-8'
        )
        print(f'wrote {args.ftxui_logo_data}')
        return

    if args.ftxui_intro_data is not None:
        args.ftxui_intro_data.parent.mkdir(parents=True, exist_ok=True)
        args.ftxui_intro_data.write_text(
            emit_ftxui_intro_data(Path(args.image)), encoding='utf-8'
        )
        print(f'wrote {args.ftxui_intro_data}')
        return

    cells = build_cells(args)

    if not args.no_preview:
        print_preview(cells)

    generated = cpp_rows(cells)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(generated, encoding="utf-8")
    else:
        print()
        print(generated, end="")

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json_cells(cells), encoding="utf-8")


if __name__ == "__main__":
    main()
