#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
文件编码转换脚本
将指定路径下所有指定后缀的文件编码格式改为UTF-8带签名（BOM）格式，支持递归修改
仅使用Python标准库，无需安装第三方依赖
"""

import os
import sys
import argparse
from pathlib import Path

def detect_encoding(file_path):
    """
    检测文件编码（使用常见编码尝试解码）
    :param file_path: 文件路径
    :return: 编码名称
    """
    # 常见编码列表，按优先级排序
    encodings = [
        'utf-8-sig',  # UTF-8 with BOM
        'utf-8',      # UTF-8
        'gbk',        # 中文GBK
        'gb2312',     # 中文GB2312
        'utf-16',     # UTF-16
        'utf-16le',   # UTF-16 Little Endian
        'utf-16be',   # UTF-16 Big Endian
        'latin1',     # Latin-1
        'cp1252',     # Windows-1252
        'ascii'       # ASCII
    ]
    
    try:
        with open(file_path, 'rb') as f:
            raw_data = f.read()
        
        # 检查是否为空文件
        if not raw_data:
            return 'utf-8'
        
        # 检查UTF-8 BOM
        if raw_data.startswith(b'\xef\xbb\xbf'):
            return 'utf-8-sig'
        
        # 检查UTF-16 BOM
        if raw_data.startswith(b'\xff\xfe'):
            return 'utf-16le'
        if raw_data.startswith(b'\xfe\xff'):
            return 'utf-16be'
        
        # 尝试各种编码
        for encoding in encodings:
            try:
                raw_data.decode(encoding)
                return encoding
            except (UnicodeDecodeError, UnicodeError):
                continue
        
        # 如果所有编码都失败，返回latin1（几乎总能解码）
        return 'latin1'
        
    except Exception as e:
        print(f"检测文件编码失败 {file_path}: {e}")
        return 'utf-8'

def has_bom(file_path):
    """
    检查文件是否已经有UTF-8 BOM
    :param file_path: 文件路径
    :return: 是否有BOM
    """
    try:
        with open(file_path, 'rb') as f:
            first_bytes = f.read(3)
            return first_bytes == b'\xef\xbb\xbf'
    except Exception:
        return False

def is_binary_file(file_path):
    """
    简单检测是否为二进制文件
    :param file_path: 文件路径
    :return: 是否为二进制文件
    """
    try:
        with open(file_path, 'rb') as f:
            chunk = f.read(1024)
            if b'\x00' in chunk:  # 包含空字节，可能是二进制文件
                return True
            # 检查是否有太多非打印字符
            text_chars = bytearray({7,8,9,10,12,13,27} | set(range(0x20, 0x100)) - {0x7f})
            return bool(chunk.translate(None, text_chars))
    except Exception:
        return True

def convert_to_utf8_bom(file_path, original_encoding=None):
    """
    将文件转换为UTF-8带BOM格式
    :param file_path: 文件路径
    :param original_encoding: 原始编码，如果为None则自动检测
    :return: 转换是否成功
    """
    try:
        # 如果已经是UTF-8 BOM，跳过
        if has_bom(file_path):
            return 'skip'
        
        # 检查是否为二进制文件
        if is_binary_file(file_path):
            return 'binary'
        
        # 检测原始编码
        if not original_encoding:
            original_encoding = detect_encoding(file_path)
        
        if not original_encoding:
            return 'no_encoding'
        
        # 读取原文件内容
        with open(file_path, 'r', encoding=original_encoding, errors='ignore') as f:
            content = f.read()
        
        # 写入UTF-8 BOM格式
        with open(file_path, 'w', encoding='utf-8-sig') as f:
            f.write(content)
        
        return 'success'
        
    except Exception as e:
        print(f"转换文件失败 {file_path}: {e}")
        return 'error'

def find_files_by_extension(root_path, extensions):
    """
    递归查找指定后缀的文件
    :param root_path: 根路径
    :param extensions: 文件后缀列表
    :return: 文件路径生成器
    """
    root_path = Path(root_path)
    
    # 确保extensions是列表格式且以点开头
    if isinstance(extensions, str):
        extensions = [extensions]
    
    extensions = [ext if ext.startswith('.') else f'.{ext}' for ext in extensions]
    
    for file_path in root_path.rglob('*'):
        if file_path.is_file() and file_path.suffix.lower() in [ext.lower() for ext in extensions]:
            yield file_path

def convert_files(root_path, extensions, dry_run=False, verbose=False):
    """
    批量转换文件编码
    :param root_path: 根路径
    :param extensions: 文件后缀列表
    :param dry_run: 是否为试运行模式
    :param verbose: 是否详细输出
    :return: 转换统计信息
    """
    stats = {
        'total': 0,
        'success': 0,
        'failed': 0,
        'skipped': 0,
        'binary': 0
    }
    
    print(f"开始{'试运行' if dry_run else '转换'}，路径: {root_path}，后缀: {extensions}")
    
    for file_path in find_files_by_extension(root_path, extensions):
        stats['total'] += 1
        
        if dry_run:
            current_encoding = detect_encoding(file_path)
            has_utf8_bom = has_bom(file_path)
            is_binary = is_binary_file(file_path)
            
            if is_binary:
                status = "二进制文件(跳过)"
            elif has_utf8_bom:
                status = "已有BOM"
            else:
                status = f"需转换({current_encoding})"
            
            if verbose:
                print(f"[试运行] {file_path} - {status}")
            continue
        
        result = convert_to_utf8_bom(file_path)
        
        if result == 'success':
            stats['success'] += 1
            if verbose:
                print(f"✓ 转换成功: {file_path}")
        elif result == 'skip':
            stats['skipped'] += 1
            if verbose:
                print(f"- 跳过(已有BOM): {file_path}")
        elif result == 'binary':
            stats['binary'] += 1
            if verbose:
                print(f"- 跳过(二进制): {file_path}")
        else:
            stats['failed'] += 1
            if verbose:
                print(f"✗ 转换失败: {file_path}")
    
    return stats

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='将指定路径下所有指定后缀的文件编码格式改为UTF-8带签名格式',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
使用示例:
  %(prog)s /path/to/files .cpp .h              # 转换C++文件
  %(prog)s /path/to/files .txt --dry-run       # 试运行模式
  %(prog)s /path/to/files .py .js .html        # 转换多种类型文件
  %(prog)s /path/to/files .cpp --verbose       # 详细输出
        """
    )
    
    parser.add_argument('path', help='要处理的根路径')
    parser.add_argument('extensions', nargs='+', help='要处理的文件后缀（如 .cpp .h .txt）')
    parser.add_argument('--dry-run', action='store_true', help='试运行模式，只显示将要处理的文件，不实际修改')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细输出')
    
    args = parser.parse_args()
    
    # 验证路径
    if not os.path.exists(args.path):
        print(f"错误: 路径不存在: {args.path}")
        sys.exit(1)
    
    try:
        # 执行转换
        stats = convert_files(args.path, args.extensions, args.dry_run, args.verbose)
        
        # 输出统计信息
        mode_text = "试运行完成" if args.dry_run else "转换完成"
        print(f"\n{mode_text}:")
        print(f"总文件数: {stats['total']}")
        if not args.dry_run:
            print(f"成功转换: {stats['success']}")
            print(f"转换失败: {stats['failed']}")
            print(f"跳过文件(已有BOM): {stats['skipped']}")
            print(f"跳过文件(二进制): {stats['binary']}")
        
    except KeyboardInterrupt:
        print("\n用户中断操作")
        sys.exit(1)
    except Exception as e:
        print(f"程序执行出错: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()