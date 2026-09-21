# -*- coding: utf-8 -*-
"""
Công cụ trích xuất video chuẩn RGB565 480x272 cho STM32F746G-DISCO
Hỗ trợ: Video (.mp4, .avi, .mkv, .mov) và Hoạt hình (.gif)
Yêu cầu: pip install opencv-python numpy
"""

import sys
import os
import argparse

if hasattr(sys.stdout, 'reconfigure'):
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass

def convert_with_opencv(input_path, output_path, target_fps=None):
    try:
        import cv2
        import numpy as np
    except ImportError:
        print("[ERROR] Thieu thu vien opencv-python!")
        print("Vui long chay: pip install opencv-python numpy")
        return False

    cap = cv2.VideoCapture(input_path)
    if not cap.isOpened():
        print(f"[ERROR] Khong the mo video: {input_path}")
        return False

    src_fps = cap.get(cv2.CAP_PROP_FPS)
    total_src_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    print(f"[*] Dang xu ly: {input_path}")
    print(f"[*] FPS nguon: {src_fps:.2f} | Tong so frame nguon: {total_src_frames}")

    skip_ratio = 1.0
    if target_fps is not None and target_fps > 0 and target_fps < src_fps:
        skip_ratio = src_fps / target_fps
        print(f"[*] FPS xuat: {target_fps} (Lay mau ti le: {skip_ratio:.2f})")
    else:
        target_fps = int(round(src_fps)) if src_fps > 0 else 30
        print(f"[*] FPS xuat: {target_fps} (Giu nguyen)")

    print(f"[*] Do phan giai xuat: 480x272 | Dinh dang: RGB565 Little-Endian (2 bytes/pixel)")

    frame_size_bytes = 480 * 272 * 2  # 261,120 bytes
    out_frames = 0
    current_src_idx = 0.0

    with open(output_path, "wb") as f_out:
        while True:
            ret, frame = cap.read()
            if not ret:
                break

            if skip_ratio > 1.0:
                if (cap.get(cv2.CAP_PROP_POS_FRAMES) < current_src_idx):
                    continue
                current_src_idx += skip_ratio

            resized = cv2.resize(frame, (480, 272), interpolation=cv2.INTER_AREA)
            rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)

            r = (rgb[:, :, 0] >> 3).astype(np.uint16)
            g = (rgb[:, :, 1] >> 2).astype(np.uint16)
            b = (rgb[:, :, 2] >> 3).astype(np.uint16)
            rgb565 = (r << 11) | (g << 5) | b

            f_out.write(rgb565.tobytes())
            out_frames += 1

            if out_frames % 50 == 0:
                print(f"  -> Da xuat {out_frames} frames...", end="\r")

    cap.release()
    file_size_mb = (out_frames * frame_size_bytes) / (1024 * 1024)
    print(f"\n[OK] Hoan tat! Xuat thanh cong {out_frames} frames sang {output_path} ({file_size_mb:.2f} MB)")
    print(f"[*] Thoi luong phat: {out_frames / target_fps:.1f} giay o toc do {target_fps} FPS.")
    print("[*] Huong dan: Copy file nay vao the MicroSD FAT32 va cam vao kit STM32F746G-DISCO.")
    return True

def main():
    parser = argparse.ArgumentParser(description="Chuyen doi video sang chuan RGB565 480x272 cho STM32F746G-DISCO")
    parser.add_argument("input", help="Duong dan file video nguon (.mp4, .avi, .gif...)")
    parser.add_argument("output", nargs="?", default="VIDEO1.BIN", help="Ten file binary dau ra (mac dinh: VIDEO1.BIN)")
    parser.add_argument("--fps", type=int, default=None, help="Toc do khung hinh mong muon (vi du: 30 hoac 60)")

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"[ERROR] Khong tim thay file: {args.input}")
        return

    convert_with_opencv(args.input, args.output, args.fps)

if __name__ == "__main__":
    main()
