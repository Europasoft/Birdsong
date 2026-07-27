# Copyright © 2026 Simon Liimatainen
import os
from subprocess import Popen, PIPE
from sys import exit as sys_exit, argv as sys_args
from time import sleep
import csv
from pathlib import Path

SRC_DIR = "shadercode"
COMPILER = "D:/VulkanDev/VulkanSDK/1.3.246.1/Bin/glslc.exe"
OUT_DIR = "compiled"

def load_shader_list(filepath):
	shaders = []
	try:
		with open(filepath) as fp:
			reader = csv.reader(fp, delimiter=",", quotechar='"')
			for l in reader:
				for shader in l:
					shaders.append(''.join(str(shader).split())) # removes whitespace
	except Exception:
		return None
	shaders = [s.strip() for s in shaders]
	shaders = [s for s in shaders if s]
	return shaders



class Results():
	def __init__(self):
		self.compiled = []
		self.notfound = []
		self.failed = []
	def combine(self, other):
		self.compiled.extend(other.compiled)
		self.notfound.extend(other.notfound)
		self.failed.extend(other.failed)

def color_green(s):
	return "\033[32m" + s + "\033[0m"
def color_red(s):
	return "\033[31m" + s + "\033[0m"
def color_yellow(s):
	return "\033[33m" + s + "\033[0m"

def compile_shader(shader_name, dir, compiler_path, out_dir_full_path):
	src = shader_name
	dst = f"{src}.spv" # SPIR-V filename
	src = os.path.join(dir, src)
	dst = os.path.join(out_dir_full_path, dst)

	result = Results()

	if (not os.path.isfile(src)):
		print(color_yellow(f"     {src} not found"))
		result.notfound.append(src)
		return result
	else:
		# COMPILE
		compiler = Popen([compiler_path, src, '-o', dst], stdout=PIPE, stderr=PIPE)
		stdout, stderr = compiler.communicate()

		if stderr:
			print(color_red(f"     {src} failed to compile"))
			result.failed.append((src, stderr.decode())) # compiler error
			return result
		else:
			print(color_green(f"[OK] {src}"))
			result.compiled.append(dst)
			return result

def run(shaders_dir, compiler_path, shaders):
	if not os.path.isfile(compiler_path):
		print(color_red(f"Compiler executable not found in {compiler_path}"))
		return False
	if not os.path.isdir(shaders_dir):
		print(color_red(f"Cannot find path {dir}"))
		return False

	out_dir_full_path = os.path.abspath(OUT_DIR)
	if (not os.path.isdir(out_dir_full_path)):
		Path(out_dir_full_path).mkdir(parents=True, exist_ok=True)
		print(f"Created new directory at {out_dir_full_path}")

	results = Results()
	for shader in shaders:
		results.combine(compile_shader(shader, shaders_dir, compiler_path, out_dir_full_path))

	res = str()
	if len(results.failed) > 0:
		res += color_red(f"{len(results.failed)} failed")

	if len(results.notfound) > 0:
		if res: res += ", "
		res += color_yellow(f"{len(results.notfound)} not found")

	if len(results.compiled) > 0:
		if res: res += ", "
		res += color_green(f"{len(results.compiled)} compiled")

	print("Shaders: " + res)

	if results.failed:
		for f in results.failed:
			print(color_red(f[0] + " - " + f[1]))
		return False

	if len(results.compiled) >= len(shaders):
		print(color_green("\nCompleted successfully\n"))
		return True


if __name__ == '__main__':
	if (len(sys_args) >= 2):
		SRC_DIR = sys_args[1]
	if (len(sys_args) >= 3):
		COMPILER = sys_args[2]

	shaders_file = "compile.csv"
	shaders = load_shader_list(shaders_file)
	if not shaders:
		print(color_red(f"Failed to load shader list {os.path.join(SRC_DIR, shaders_file)}"))
		sys_exit(1)
	print(f"{len(shaders)} shaders")

	if not run(SRC_DIR, COMPILER, shaders): 
		sys_exit(1)