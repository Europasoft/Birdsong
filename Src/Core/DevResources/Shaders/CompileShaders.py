# Copyright © 2026 Simon Liimatainen
import os
from subprocess import Popen, PIPE
from sys import exit as sys_exit, argv as sys_args
from time import sleep
import csv

DEFAULT_DIR = "D:/source/repos/Birdsong/Src/Core/DevResources/Shaders"
DEFAULT_COMPILER = "D:/VulkanDev/VulkanSDK/1.3.246.1/Bin/glslc.exe"


def load_shader_list(filepath):
    shaders = []
    try:
        with open(filepath) as fp:
            reader = csv.reader(fp, delimiter=",", quotechar='"')
            for l in reader:
                for shader in l:
                    shaders.append(''.join(str(shader).split())) # removes whitespace
                    print(shaders[-1])
    except Exception:
        return None
    return shaders



completed = []
skipped = []
failed = []

def compile_shader(shader_name, shader_type, dir, compiler_path):
    src = "{}.{}".format(shader_name, shader_type) # GLSL filename
    dst = "{}.spv".format(src) # SPIR-V filename
    src = os.path.join(dir, src)
    dst = os.path.join(dir, dst)

    if not os.path.isfile(src):
        skipped.append(src)
        return

    compiler = Popen([compiler_path, src, '-o', dst], stdout=PIPE, stderr=PIPE)
    stdout, stderr = compiler.communicate()

    if stderr:
        failed.append((src, stderr.decode())) # compiler error
    else:
        completed.append(dst)


def vs_error(filename, error):
    print("{} : error GLSL: Failed to compile shader: {}".format(filename, error))

def vs_warning(filename, warning):
    print("{} : warning GLSL: Failed to compile shader: {}".format(filename, warning))


def run(shaders_dir, compiler_path, shaders):
    if not os.path.isfile(compiler_path):
        vs_error("Shader compiler", "Compiler executable not found in {}".format(compiler_path))
        return False
    if not os.path.isdir(shaders_dir):
        vs_error("Shader compiler", "Cannot find path {}".format(dir))
        return False

    for shader in shaders:
        compile_shader(shader, 'frag', shaders_dir, compiler_path)
        compile_shader(shader, 'vert', shaders_dir, compiler_path)

    res = str()
    if failed:
        res += "{n} failed".format(n=len(failed))
    if skipped:
        if res: res += ", "
        res += "{n} skipped".format(n=len(skipped))
    if completed:
        if res: res += ", "
        res += "{n} compiled".format(n=len(completed))
    print("Shaders: " + res)

    if len(skipped) >= len(shaders):
        vs_warning("Shader compiler", "All skipped, ensure the path is correct")

    if failed:
        for f in failed:
            vs_error(f[0], f[1])
        return False
    else:
        print("Shader compilation successfully completed")
        return True


if __name__ == '__main__':
    compiler_path = DEFAULT_COMPILER
    dir = DEFAULT_DIR

    if (len(sys_args) >= 2):
        dir = sys_args[1]
    if (len(sys_args) >= 3):
        compiler_path = sys_args[2]

    shaders_file = "compile.csv"
    shaders = load_shader_list(os.path.join(dir, shaders_file))
    if not shaders:
        vs_error("Shader compiler", "Failed to load shader list {}".format(os.path.join(dir, shaders_file)))
        sys_exit(1)

    if not run(dir, compiler_path, shaders): 
        sys_exit(1)