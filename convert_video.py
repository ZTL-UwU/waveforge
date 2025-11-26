#!/usr/bin/env python3
"""
Convert demo_video_frames.dat to mp4 video file

demo_video_frames.dat format:
- Consecutive RGBA frame data
- Each frame size: width * height * 4 bytes
- Frame rate: 24 FPS
"""

import argparse
import subprocess
import sys
from pathlib import Path


def convert_dat_to_webm(
    input_file: str,
    output_file: str,
    width: int,
    height: int,
    fps: int = 24,
    scale: int = 1
):
    """
    Convert raw RGBA frame data to mp4 video using ffmpeg
    
    Args:
        input_file: Path to demo_video_frames.dat file
        output_file: Path to output mp4 file
        width: Video width (pixels)
        height: Video height (pixels)
        fps: Frame rate (default 24)
        scale: Pixel-perfect scaling factor (default 1, no scaling)
    """
    input_path = Path(input_file)
    
    if not input_path.exists():
        print(f"Error: Input file does not exist: {input_file}", file=sys.stderr)
        sys.exit(1)
    
    # Calculate frame count
    file_size = input_path.stat().st_size
    frame_size = width * height * 4  # RGBA
    frame_count = file_size // frame_size
    
    if file_size % frame_size != 0:
        print(f"Warning: File size ({file_size} bytes) is not a multiple of frame size", file=sys.stderr)
        print(f"  Frame size: {frame_size} bytes ({width}x{height} RGBA)", file=sys.stderr)
        print(f"  Complete frames: {frame_count}", file=sys.stderr)
        print(f"  Remaining bytes: {file_size % frame_size}", file=sys.stderr)
    
    output_width = width * scale
    output_height = height * scale
    
    print(f"Input file: {input_file}")
    print(f"Output file: {output_file}")
    print(f"Original resolution: {width}x{height}")
    if scale > 1:
        print(f"Scale factor: {scale}x")
        print(f"Output resolution: {output_width}x{output_height}")
    else:
        print(f"Output resolution: {width}x{height}")
    print(f"Frame rate: {fps} FPS")
    print(f"Total frames: {frame_count}")
    print(f"Video duration: {frame_count / fps:.2f} seconds")
    print()
    
    # ffmpeg command
    # -f rawvideo: Specify input format as raw video
    # -pixel_format rgba: Input pixel format is RGBA
    # -video_size: Video resolution
    # -framerate: Frame rate
    # -i: Input file
    # -vf: Video filters
    #   vflip: Vertical flip (Y-axis flip)
    #   scale: Pixel-perfect scaling using nearest neighbor algorithm to keep pixels sharp
    # -c:v libx264: Use H.264 encoder (mp4 format, widely compatible)
    # -pix_fmt yuv420p: Output pixel format (no alpha channel, maximum compatibility)
    # -crf 40: Quality parameter (0-51, lower is better, 18 is visually lossless)
    
    # Build video filters
    vfilters = ['vflip']  # Y-axis flip
    if scale > 1:
        # Pixel-perfect scaling: use nearest neighbor, each pixel becomes scale x scale square
        vfilters.append(f'scale={output_width}:{output_height}:flags=neighbor')
    vfilter_str = ','.join(vfilters)
    
    cmd = [
        'ffmpeg',
        '-f', 'rawvideo',
        '-pixel_format', 'rgba',
        '-video_size', f'{width}x{height}',
        '-framerate', str(fps),
        '-i', input_file,
        '-vf', vfilter_str,
        '-c:v', 'libx264',
        '-preset', 'fast',
        '-pix_fmt', 'yuv420p',  # No alpha channel
        '-crf', '18',
        '-y',  # Overwrite output file
        output_file
    ]
    
    print("Running ffmpeg command...")
    print(' '.join(cmd))
    print()
    
    try:
        subprocess.run(cmd, check=True)
        print(f"\nConversion successful! Output file: {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"\nError: ffmpeg conversion failed", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print(f"\nError: ffmpeg command not found, please install ffmpeg first", file=sys.stderr)
        print("  Ubuntu/Debian: sudo apt install ffmpeg", file=sys.stderr)
        print("  Arch Linux: sudo pacman -S ffmpeg", file=sys.stderr)
        print("  macOS: brew install ffmpeg", file=sys.stderr)
        sys.exit(1)


def main():
    parser = argparse.ArgumentParser(
        description='Convert demo_video_frames.dat to mp4 video file',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""Examples:
  %(prog)s demo_video_frames.dat output.mp4
  %(prog)s demo_video_frames.dat output.mp4 --width 320 --height 240
  %(prog)s demo_video_frames.dat output.mp4 -W 256 -H 192 --scale 8
  %(prog)s demo_video_frames.dat output.mp4 -s 2 --fps 30
        """
    )
    
    parser.add_argument(
        'input',
        help='Input demo_video_frames.dat file'
    )
    
    parser.add_argument(
        'output',
        help='Output mp4 video file'
    )
    
    parser.add_argument(
        '-W', '--width',
        type=int,
        default=256,
        help='Video width in pixels (default: 256)'
    )
    
    parser.add_argument(
        '-H', '--height',
        type=int,
        default=192,
        help='Video height in pixels (default: 192)'
    )
    
    parser.add_argument(
        '--fps',
        type=int,
        default=24,
        help='Frame rate in FPS (default: 24)'
    )
    
    parser.add_argument(
        '-s', '--scale',
        type=int,
        default=8,
        help='Pixel-perfect scaling factor (default: 8, i.e., 256x192 becomes 2048x1536)'
    )
    
    args = parser.parse_args()
    
    # Validate parameters
    if args.width <= 0 or args.height <= 0:
        print("Error: Width and height must be positive", file=sys.stderr)
        sys.exit(1)
    
    if args.fps <= 0:
        print("Error: Frame rate must be positive", file=sys.stderr)
        sys.exit(1)
    
    if args.scale <= 0:
        print("Error: Scale factor must be positive", file=sys.stderr)
        sys.exit(1)
    
    convert_dat_to_webm(
        args.input,
        args.output,
        args.width,
        args.height,
        args.fps,
        args.scale
    )


if __name__ == '__main__':
    main()
