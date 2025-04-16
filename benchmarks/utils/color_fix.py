# 颜色处理工具函数
import sys, os

def is_terminal_output(file=None):
    """检查输出是否直接到终端"""
    if file is None:
        file = sys.stdout
    return os.isatty(file.fileno()) if hasattr(file, 'fileno') else False

class ColorFormatter:
    """根据输出目标处理颜色"""
    # ANSI颜色代码
    GREEN = '\033[0;32m' if is_terminal_output() else ''
    YELLOW = '\033[1;33m' if is_terminal_output() else ''
    RED = '\033[0;31m' if is_terminal_output() else ''
    NC = '\033[0m' if is_terminal_output() else ''  # 无颜色

    @staticmethod
    def green(text):
        return f"{ColorFormatter.GREEN}{text}{ColorFormatter.NC}"
    
    @staticmethod
    def yellow(text):
        return f"{ColorFormatter.YELLOW}{text}{ColorFormatter.NC}"
    
    @staticmethod
    def red(text):
        return f"{ColorFormatter.RED}{text}{ColorFormatter.NC}"
    
    @staticmethod
    def strip_color(text):
        """移除字符串中的ANSI颜色代码"""
        import re
        ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
        return ansi_escape.sub('', text)
