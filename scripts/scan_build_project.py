"""
file: scan_build_project.py
author: Rudy Castan
date: 2025 Fall
course: CS200 Computer Graphics I
copyright: DigiPen Institute of Technology
"""

import os
import re
import sys
import platform
import subprocess
import argparse
import time
import threading
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

TODO_PATTERN = re.compile(r'TODO|FIXME', re.IGNORECASE)
OPENGL_PATTERN = re.compile(r'\b(gl[A-Z][a-zA-Z0-9_]*)\(')

# ANSI color codes for better output
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

def colored_print(text, color=Colors.WHITE, bold=False):
    """Print colored text to stdout"""
    prefix = Colors.BOLD if bold else ""
    print(f"{prefix}{color}{text}{Colors.END}")

def print_separator(title="", char="=", width=80):
    """Print a separator line with optional title"""
    if title:
        title_with_spaces = f" {title} "
        padding = (width - len(title_with_spaces)) // 2
        line = char * padding + title_with_spaces + char * padding
        if len(line) < width:
            line += char
    else:
        line = char * width
    colored_print(line, Colors.CYAN, bold=True)

class BuildSpinner:
    """ASCII spinner animation for build progress"""
    def __init__(self):
        self.spinner_chars = ['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏']
        self.spinning = False
        self.spinner_thread = None
        self.current_message = ""
        self.start_time = None
        self.progress_info = None  # (current, total) for progress bar
        self.last_line_length = 0  # Track length of last printed line for proper clearing
        
    def _create_progress_bar(self, current, total, width=20):
        """Create a simple ASCII progress bar"""
        if total == 0:
            return ""
        
        filled = int(width * current / total)
        bar = "█" * filled + "░" * (width - filled)
        percentage = int(100 * current / total)
        return f" [{bar}] {percentage}%"
    
    def _get_terminal_width(self):
        """Get terminal width, with fallback"""
        try:
            import shutil
            return shutil.get_terminal_size().columns
        except:
            return 80  # fallback width
    
    def _clear_line_and_print(self, content):
        """Clear the current line and print new content, handling trailing characters"""
        # Calculate the display length without ANSI color codes
        plain_content = re.sub(r'\033\[[0-9;]*m', '', content)
        content_length = len(plain_content)
        
        # If the new content is shorter than the last line, pad with spaces
        terminal_width = self._get_terminal_width()
        clear_length = max(self.last_line_length, content_length)
        
        # Ensure we don't exceed terminal width
        if clear_length > terminal_width - 1:
            clear_length = terminal_width - 1
        
        # Clear line with spaces, then print content
        if content_length < self.last_line_length:
            # Need to clear trailing characters
            padded_content = content + ' ' * (self.last_line_length - content_length)
            print(f'\r{padded_content}\r{content}', end='', flush=True)
        else:
            print(f'\r{content}', end='', flush=True)
        
        self.last_line_length = content_length
        
    def _spin(self):
        """Internal spinning method"""
        idx = 0
        while self.spinning:
            char = self.spinner_chars[idx % len(self.spinner_chars)]
            
            # Add elapsed time if we have a start time
            time_str = ""
            if self.start_time:
                elapsed = time.time() - self.start_time
                time_str = f" ({elapsed:.1f}s)"
            
            # Add progress bar if we have progress info
            progress_str = ""
            if self.progress_info:
                current, total = self.progress_info
                progress_str = self._create_progress_bar(current, total)
            
            # Build the complete spinner line
            spinner_line = f'{Colors.YELLOW}{char}{Colors.END} {self.current_message}{Colors.CYAN}{progress_str}{Colors.END}{Colors.BLUE}{time_str}{Colors.END}'
            
            # Clear line and print spinner with message
            self._clear_line_and_print(spinner_line)
            time.sleep(0.1)
            idx += 1
    
    def start(self, message="Building...", progress=None):
        """Start the spinner with a message and optional progress info"""
        if not self.spinning:
            self.current_message = message
            self.progress_info = progress
            self.start_time = time.time()
            self.spinning = True
            self.spinner_thread = threading.Thread(target=self._spin, daemon=True)
            self.spinner_thread.start()
    
    def update_message(self, message, progress=None):
        """Update the spinner message and progress"""
        self.current_message = message
        if progress is not None:
            self.progress_info = progress
    
    def stop(self, final_message=None):
        """Stop the spinner and optionally print a final message"""
        if self.spinning:
            self.spinning = False
            if self.spinner_thread:
                self.spinner_thread.join(timeout=0.2)
            
            # Clear the entire line properly by overwriting with spaces
            if self.last_line_length > 0:
                clear_line = ' ' * self.last_line_length
                print(f'\r{clear_line}\r', end='', flush=True)
                self.last_line_length = 0
            
            if final_message:
                print(final_message)

def find_todo_comments(file_path):
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    return [(i + 1, line.strip()) for i, line in enumerate(lines) if TODO_PATTERN.search(line)]

def find_opengl_usage(file_path):
    if file_path.endswith('GL.cpp'):
        return []
    with open(file_path, 'r', encoding='utf-8') as file:
        lines = file.readlines()
    return [(i + 1, match) for i, line in enumerate(lines) for match in OPENGL_PATTERN.findall(line)]

def scan_file(file_path, skip_todos=False, skip_opengl=False):
    log_output = []
    todos = find_todo_comments(file_path) if not skip_todos else []
    opengl_issues = find_opengl_usage(file_path) if not skip_opengl and file_path.endswith((".cpp", ".hpp", ".h")) else []

    if todos:
        log_output.append(f"[TODO Found] {file_path}")
        for line, comment in todos:
            log_output.append(f"  Line {line}: {comment}")

    if opengl_issues:
        log_output.append(f"[Bad OpenGL Usage] {file_path}")
        for line, func in opengl_issues:
            log_output.append(f"  Line {line}: {func} should be wrapped (e.g., GL::{func[2:]})")

    if log_output:
        colored_print("\n".join(log_output), Colors.YELLOW if todos else Colors.RED)

    return bool(todos), bool(opengl_issues)

def scan_directory(directory, skip_todos=False, skip_opengl=False):
    if skip_todos and skip_opengl:
        colored_print("⏭️  Skipping all scanning (--skip-todos and --skip-opengl flags used)", Colors.YELLOW)
        return
        
    if skip_todos:
        colored_print("⏭️  Skipping TODO scan (--skip-todos flag used)", Colors.YELLOW)
    
    if skip_opengl:
        colored_print("⏭️  Skipping OpenGL scan (--skip-opengl flag used)", Colors.YELLOW)
        
    has_todo, has_opengl_issue = False, False
    scan_title = "TODO/FIXME"
    if not skip_todos and not skip_opengl:
        scan_title = "TODO/FIXME & OpenGL SCAN"
    elif not skip_todos:
        scan_title = "TODO/FIXME SCAN"
    elif not skip_opengl:
        scan_title = "OpenGL SCAN"
    
    if not skip_todos or not skip_opengl:
        print_separator(scan_title)
        colored_print(f"📁 Scanning directory: {directory}", Colors.BLUE, bold=True)
        print()
    
    scanned_files = 0
    with ThreadPoolExecutor() as executor:
        futures = []
        for root, _, files in os.walk(directory):
            if "build" in root:
                continue
            for file in files:
                if file.endswith((".cpp", ".hpp", ".h", ".vert", ".frag", ".cmake", ".txt", ".html")):
                    file_path = os.path.join(root, file)
                    futures.append(executor.submit(scan_file, file_path, skip_todos, skip_opengl))
                    scanned_files += 1
        
        for future in futures:
            todo_found, opengl_issue_found = future.result()
            has_todo |= todo_found
            has_opengl_issue |= opengl_issue_found
    
    if has_todo:
        colored_print(f"\n❌ TODO comments found in {scanned_files} scanned files. Fix them before proceeding.", Colors.RED, bold=True)
    
    if has_opengl_issue:
        colored_print(f"\n❌ Bad OpenGL usage found in {scanned_files} scanned files. Fix them before proceeding.", Colors.RED, bold=True)

    if has_todo or has_opengl_issue:
        sys.exit(1)
    elif not skip_todos or not skip_opengl:
        success_msg = "✅ No "
        if not skip_todos and not skip_opengl:
            success_msg += "TODO/FIXME comments or bad OpenGL usage"
        elif not skip_todos:
            success_msg += "TODO/FIXME comments"
        elif not skip_opengl:
            success_msg += "bad OpenGL usage"
        success_msg += f" found in {scanned_files} scanned files!"
        colored_print(success_msg, Colors.GREEN, bold=True)

def windows_to_wsl_path(win_path: str) -> str:
    win_path = os.path.abspath(win_path)
    match = re.match(r"([A-Za-z]):\\(.*)", win_path)
    if not match:
        raise ValueError("Invalid Windows path format")
    drive, path = match.groups()
    return "/mnt/" + drive.lower() + "/" + path.replace("\\", "/")


def run_cmake_command(folder, build_type, target):
    linux_prefix = []
    build_dir = os.path.join(folder, f'build/{target}-{build_type.lower()}')

    if platform.system() == "Windows" and (target == "web" or target == "linux"):
        linux_prefix = ['wsl']
        folder = windows_to_wsl_path(folder)
        build_dir = windows_to_wsl_path(build_dir)
    
    start_time = time.time()
    
    # Configure step
    configure_start = time.time()
    cmake_configure = subprocess.run(linux_prefix + ['cmake', '--preset', f'{target}-{build_type.lower()}', '-S', folder], capture_output=True, text=True, encoding="utf-8")
    configure_time = time.time() - configure_start
    
    if cmake_configure.returncode != 0:
        return cmake_configure.stderr + '\n' + cmake_configure.stdout, build_type, target, 0, configure_time, 0
    
    # Build step
    build_start = time.time()
    config_build_type = build_type if build_type != "developer-release" else "Release"
    cmake_build = subprocess.run(linux_prefix + ['cmake', '--build', build_dir, '--config', config_build_type], capture_output=True, text=True, encoding="utf-8")
    build_time = time.time() - build_start
    total_time = time.time() - start_time
    
    if cmake_build.returncode != 0:
        return cmake_build.stderr + '\n' + cmake_build.stdout, build_type, target, total_time, configure_time, build_time
    
    return "", build_type, target, total_time, configure_time, build_time

def get_executable_info(folder, build_type, target):
    """Get information about the built executable and how to run it"""
    build_dir = Path(folder) / f'build/{target}-{build_type.lower()}'
    
    if target == "web":
        # Web builds create HTML files
        html_file = build_dir / "cs200_fun.html"
        if html_file.exists():
            return {
                'type': 'web',
                'path': str(html_file),
                'run_command': f"python3 -m http.server 8000 # Then open http://localhost:8000/{html_file.name}",
                'directory': str(build_dir)
            }
    else:
        # Native builds create executables
        exe_name = "cs200_fun.exe" if platform.system() == "Windows" else "cs200_fun"
        exe_file = build_dir / exe_name
        if exe_file.exists():
            return {
                'type': 'executable',
                'path': str(exe_file),
                'run_command': f"cd {build_dir} && ./{exe_name}",
                'directory': str(build_dir)
            }
    
    return None

def build_project(folder, target_filter=None, build_type_filter=None):
    print_separator("BUILD PROCESS")
    colored_print("🔨 Starting build process...", Colors.BLUE, bold=True)
    
    error_output = []
    build_results = []
    total_start_time = time.time()
    
    build_types = ['debug', 'developer-release', 'release']
    targets = ['windows', 'web', 'linux'] if platform.system() == "Windows" else ['linux', 'web']
    
    # Apply filters
    if target_filter:
        targets = [t for t in targets if t in target_filter]
    if build_type_filter:
        build_types = [b for b in build_types if b in build_type_filter]
    
    total_builds = len(targets) * len(build_types)
    
    # Start a global spinner for the entire build process
    spinner = BuildSpinner()
    spinner.start(f"Building {total_builds} configuration{'s' if total_builds > 1 else ''}...", progress=(0, total_builds))
    
    with ThreadPoolExecutor() as executor:
        futures = []
        current_build = 0
        
        for target in targets:
            for build_type in build_types:
                current_build += 1
                build_name = f"{target.upper()} {build_type}"
                future = executor.submit(run_cmake_command, folder, build_type, target)
                futures.append((future, target, build_type, current_build, build_name))
                
        completed_builds = 0
        for future, target, build_type, build_number, build_name in futures:
            # Update spinner with current progress
            spinner.update_message(f"Building {build_name}...", progress=(completed_builds, total_builds))
            
            error_messages, returned_build_type, returned_target, total_time, configure_time, build_time = future.result()
            completed_builds += 1
            
            # Temporarily stop spinner to show result
            spinner.stop()
            
            if error_messages:
                colored_print(f"❌ [{build_number}/{total_builds}] {returned_target.upper()} {returned_build_type} build failed ({total_time:.1f}s)", Colors.RED, bold=True)
                # Store structured error information for better reporting
                error_output.append({
                    'target': returned_target,
                    'build_type': returned_build_type,
                    'build_number': build_number,
                    'total_builds': total_builds,
                    'time': total_time,
                    'configure_time': configure_time,
                    'build_time': build_time,
                    'error_messages': error_messages
                })
                build_results.append({
                    'target': returned_target,
                    'build_type': returned_build_type,
                    'success': False,
                    'time': total_time,
                    'configure_time': configure_time,
                    'build_time': build_time
                })
            else:
                colored_print(f"✅ [{build_number}/{total_builds}] {returned_target.upper()} {returned_build_type} built successfully ({total_time:.1f}s)", Colors.GREEN, bold=True)
                build_results.append({
                    'target': returned_target,
                    'build_type': returned_build_type,
                    'success': True,
                    'time': total_time,
                    'configure_time': configure_time,
                    'build_time': build_time
                })
            
            # Restart spinner if there are more builds
            if completed_builds < total_builds:
                remaining = total_builds - completed_builds
                spinner.start(f"Continuing with {remaining} remaining build{'s' if remaining > 1 else ''}...", progress=(completed_builds, total_builds))
    
    # Make sure spinner is stopped
    spinner.stop()
    
    total_build_time = time.time() - total_start_time
    
    # Print build summary
    print_separator("BUILD SUMMARY")
    successful_builds = [r for r in build_results if r['success']]
    failed_builds = [r for r in build_results if not r['success']]
    
    colored_print(f"📊 Total builds: {len(build_results)}", Colors.BLUE, bold=True)
    colored_print(f"✅ Successful: {len(successful_builds)}", Colors.GREEN, bold=True)
    colored_print(f"❌ Failed: {len(failed_builds)}", Colors.RED, bold=True)
    colored_print(f"⏱️  Total time: {total_build_time:.1f}s", Colors.BLUE, bold=True)
    
    if successful_builds:
        print_separator("HOW TO RUN YOUR BUILDS")
        
        for result in successful_builds:
            exe_info = get_executable_info(folder, result['build_type'], result['target'])
            if exe_info:
                colored_print(f"\n🎯 {result['target'].upper()} {result['build_type']}", Colors.CYAN, bold=True)
                
                if exe_info['type'] == 'web':
                    colored_print(f"   📄 HTML file: {exe_info['path']}", Colors.WHITE)
                    colored_print(f"   🌐 To run: cd {exe_info['directory']}", Colors.YELLOW)
                    colored_print(f"           python3 -m http.server 8000", Colors.YELLOW)
                    colored_print(f"           # Then open: http://localhost:8000/cs200_fun.html", Colors.YELLOW)
                else:
                    colored_print(f"   📱 Executable: {exe_info['path']}", Colors.WHITE)
                    colored_print(f"   ▶️  To run: {exe_info['run_command']}", Colors.YELLOW)
        
        # Print quick commands section
        print_separator("QUICK RUN COMMANDS")
        colored_print("Copy and paste these commands to run your builds:", Colors.BLUE, bold=True)
        print()
        
        for result in successful_builds:
            exe_info = get_executable_info(folder, result['build_type'], result['target'])
            if exe_info:
                if exe_info['type'] == 'web':
                    colored_print(f"# {result['target'].upper()} {result['build_type']} (Web)", Colors.CYAN)
                    colored_print(f"cd {exe_info['directory']} && python3 -m http.server 8000", Colors.GREEN)
                    print()
                else:
                    colored_print(f"# {result['target'].upper()} {result['build_type']} (Native)", Colors.CYAN)
                    colored_print(f"{exe_info['run_command']}", Colors.GREEN)
                    print()
    
    if error_output:
        print_separator("BUILD ERRORS")
        colored_print(f"⚠️  {len(error_output)} build{'s' if len(error_output) > 1 else ''} failed. Details below:", Colors.RED, bold=True)
        print()
        
        for i, error_info in enumerate(error_output, 1):
            # Header for each error
            colored_print(f"┌─ Error {i}/{len(error_output)}: {error_info['target'].upper()} {error_info['build_type']}", Colors.RED, bold=True)
            colored_print(f"│  Build time: {error_info['time']:.1f}s (configure: {error_info['configure_time']:.1f}s, compile: {error_info['build_time']:.1f}s)", Colors.YELLOW)
            colored_print("│", Colors.RED)
            
            # Split error messages into lines and format them
            error_lines = error_info['error_messages'].strip().split('\n')
            for line in error_lines:
                if line.strip():  # Skip empty lines
                    # Highlight different types of error messages
                    if 'error:' in line.lower() or 'fatal error:' in line.lower():
                        colored_print(f"│  {line}", Colors.RED)
                    elif 'warning:' in line.lower():
                        colored_print(f"│  {line}", Colors.YELLOW)
                    elif 'CMake Error' in line or 'Error' in line:
                        colored_print(f"│  {line}", Colors.RED)
                    elif line.startswith('  ') or line.startswith('\t'):
                        # Indented lines (usually context)
                        colored_print(f"│  {line}", Colors.WHITE)
                    else:
                        # Regular lines
                        colored_print(f"│  {line}", Colors.WHITE)
            
            # Footer for each error
            if i < len(error_output):
                colored_print("└─" + "─" * 70, Colors.RED)
                print()  # Extra spacing between errors
            else:
                colored_print("└─" + "─" * 70, Colors.RED)
        
        print()
        colored_print("💡 Tips for fixing build errors:", Colors.CYAN, bold=True)
        colored_print("   • Check the first error message - later ones might be cascading", Colors.CYAN)
        colored_print("   • Look for 'error:' and 'fatal error:' lines (highlighted in red above)", Colors.CYAN)
        colored_print("   • Make sure all dependencies are installed", Colors.CYAN)
        colored_print("   • Try cleaning build directory: rm -rf build/", Colors.CYAN)

def main():
    parser = argparse.ArgumentParser(
        description="Scans a project directory for TODOs and builds it using CMake.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                                    # Scan and build all configurations
  %(prog)s --skip-todos                       # Skip TODO scan, build all
  %(prog)s --skip-opengl                      # Skip OpenGL scan, build all
  %(prog)s --skip-build                       # Scan only, no building
  %(prog)s --skip-todos --skip-opengl         # Skip all scans, build only
  %(prog)s --skip-build --skip-opengl         # TODO scan only
  %(prog)s --target linux web                # Build only linux and web targets
  %(prog)s --build-type debug release        # Build only debug and release configurations
  %(prog)s --target web --build-type debug   # Build only web debug configuration
        """
    )
    parser.add_argument("directory", nargs='?', default=os.getcwd(), 
                        help="Project directory (default: current working directory)")
    parser.add_argument("--skip-todos", action="store_true", 
                        help="Skip TODO/FIXME scanning")
    parser.add_argument("--skip-opengl", action="store_true", 
                        help="Skip OpenGL usage scanning")
    parser.add_argument("--skip-build", action="store_true", 
                        help="Skip building the project (scan only)")
    parser.add_argument("--target", nargs="+", choices=['linux', 'windows', 'web'],
                        help="Build only specified targets")
    parser.add_argument("--build-type", nargs="+", choices=['debug', 'developer-release', 'release'],
                        help="Build only specified build types")
    
    args = parser.parse_args()
    
    print_separator("CS200 PROJECT BUILD SCRIPT")
    colored_print(f"🚀 Starting project analysis and build", Colors.BLUE, bold=True)
    colored_print(f"📁 Working directory: {args.directory}", Colors.WHITE)
    
    scan_directory(args.directory, skip_todos=args.skip_todos, skip_opengl=args.skip_opengl)
    
    if not args.skip_build:
        build_project(args.directory, target_filter=args.target, build_type_filter=args.build_type)
    else:
        colored_print("\n⏭️  Skipping build process (--skip-build flag used)", Colors.YELLOW, bold=True)

if __name__ == "__main__":
    if sys.stdout.encoding.lower() != "utf-8":
        sys.stdout.reconfigure(encoding="utf-8")
    if sys.stderr.encoding.lower() != "utf-8":
        sys.stderr.reconfigure(encoding="utf-8")
    main()
