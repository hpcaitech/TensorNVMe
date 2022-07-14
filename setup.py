# import os
# # from setuptools import setup, find_packages
# from torch.utils.cpp_extension import CppExtension, BuildExtension
# #
# this_dir = os.path.dirname(os.path.abspath(__file__))
# #
# #
# def cpp_ext_helper(name, sources, **kwargs):
#     return CppExtension(
#         name,
#         [os.path.join(this_dir, path) for path in sources],
#         include_dirs=[os.path.join(this_dir, 'csrc'), os.path.join(this_dir, 'include')],
#         **kwargs
#     )
#
#
# extra_compile_args = []
# if os.environ.get('DISABLE_URING') == '1':
#     extra_compile_args.append('-DDISABLE_URING')
# if os.environ.get('DISABLE_AIO') == '1':
#     extra_compile_args.append('-DDISABLE_AIO')
#
# setup(
#     name='colo_nvme',
#     packages=find_packages(exclude=(
#         'csrc',
#         'tests',
#         '*.egg-info'
#     )),
#     ext_modules=[cpp_ext_helper('colo_nvme._C', ['csrc/offload.cpp', 'csrc/uring.cpp', 'csrc/aio.cpp', 'csrc/space_mgr.cpp'],
#                                 extra_compile_args=extra_compile_args,
#                                 libraries=['uring', 'aio'])],
#     cmdclass={'build_ext': BuildExtension}
# )



# from setuptools import setup, Extension
#
# with open("stub.c", "w") as f:
#     f.write("\n")
#
#
# setup(name='colo_nvme',
#       version='@TURBO_TRANSFORMERS_VERSION@',
#       description='turbo_transformers',
#       packages=[
#           'colo_nvme',
#       ],
#       package_data={
#           '': [
#               '*.so',
#               '*.pyi',
#           ]
#       },
#       ext_modules=[Extension('colo_nvme._C', ['csrc/offload.cpp', 'csrc/uring.cpp', 'csrc/aio.cpp', 'csrc/space_mgr.cpp'],
#                    include_dirs=['csrc'],
#                    libraries=['uring', 'aio'])],
#       include_package_data=True,
#       # install_requires=[
#       #     "torch",
#       #     "transformers"
#       # ]
#       )

# TODO  11
import os
import re
import sys
import platform
import subprocess
import pathlib

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    """
    自定义了 Extension 类，忽略原来的 sources、libraries 等参数，交给 CMake 来处理这些事情
    """
    def __init__(self, name, sources=None, sourcedir=''):
        if sources is None:
            sources = []
        Extension.__init__(self, name, sources=sources)
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """
    自定义了 build_ext 类，对 CMakeExtension 的实例，调用 CMake 和 Make 命令来编译它们
    """
    def run(self):
        for ext in self.extensions:
            if isinstance(ext, CMakeExtension):
                self.build_cmake(ext)
        super().run()

    def build_cmake(self, ext):
        cwd = pathlib.Path().absolute()

        build_temp = f"{pathlib.Path(self.build_temp)}/{ext.name}"
        os.makedirs(build_temp, exist_ok=True)
        extdir = pathlib.Path(self.get_ext_fullpath(ext.name))
        extdir.mkdir(parents=True, exist_ok=True)

        config = "Debug" if self.debug else "Release"
        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + str(extdir.parent.absolute()),
            "-DCMAKE_BUILD_TYPE=" + config
        ]

        build_args = [
            "--config", config,
            "--", "-j12"
        ]

        os.chdir(build_temp)
        self.spawn(["cmake", f"{str(cwd)}/{ext.name}"] + cmake_args)
        if not self.dry_run:
            self.spawn(["cmake", "--build", "."] + build_args)
        os.chdir(str(cwd))

setup(
    name='colo_nvme',
    version='0.0.1',
    author='xxx',
    author_email='xxx',
    description='xxx',
    long_description='',
    ext_modules=[CMakeExtension('.')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)




# import os
# import re
# import subprocess
# import sys
#
# from setuptools import Extension, setup
# from setuptools.command.build_ext import build_ext
#
# # Convert distutils Windows platform specifiers to CMake -A arguments
# PLAT_TO_CMAKE = {
#     "win32": "Win32",
#     "win-amd64": "x64",
#     "win-arm32": "ARM",
#     "win-arm64": "ARM64",
# }
#
#
# # A CMakeExtension needs a sourcedir instead of a file list.
# # The name must be the _single_ output extension from the CMake build.
# # If you need multiple extensions, see scikit-build.
# class CMakeExtension(Extension):
#     def __init__(self, name, sourcedir=""):
#         Extension.__init__(self, name, sources=[])
#         self.sourcedir = os.path.abspath(sourcedir)
#
#
# class CMakeBuild(build_ext):
#     def build_extension(self, ext):
#         extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
#
#         # required for auto-detection & inclusion of auxiliary "native" libs
#         if not extdir.endswith(os.path.sep):
#             extdir += os.path.sep
#
#         debug = int(os.environ.get("DEBUG", 0)) if self.debug is None else self.debug
#         cfg = "Debug" if debug else "Release"
#
#         # CMake lets you override the generator - we need to check this.
#         # Can be set with Conda-Build, for example.
#         cmake_generator = os.environ.get("CMAKE_GENERATOR", "")
#
#         # Set Python_EXECUTABLE instead if you use PYBIND11_FINDPYTHON
#         # EXAMPLE_VERSION_INFO shows you how to pass a value into the C++ code
#         # from Python.
#         cmake_args = [
#             f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
#             f"-DPYTHON_EXECUTABLE={sys.executable}",
#             f"-DCMAKE_BUILD_TYPE={cfg}",  # not used on MSVC, but no harm
#         ]
#         build_args = []
#         # Adding CMake arguments set as environment variable
#         # (needed e.g. to build for ARM OSx on conda-forge)
#         if "CMAKE_ARGS" in os.environ:
#             cmake_args += [item for item in os.environ["CMAKE_ARGS"].split(" ") if item]
#
#         # In this example, we pass in the version to C++. You might not need to.
#         cmake_args += [f"-DEXAMPLE_VERSION_INFO={self.distribution.get_version()}"]
#
#         if self.compiler.compiler_type != "msvc":
#             # Using Ninja-build since it a) is available as a wheel and b)
#             # multithreads automatically. MSVC would require all variables be
#             # exported for Ninja to pick it up, which is a little tricky to do.
#             # Users can override the generator with CMAKE_GENERATOR in CMake
#             # 3.15+.
#             if not cmake_generator or cmake_generator == "Ninja":
#                 try:
#                     import ninja  # noqa: F401
#
#                     ninja_executable_path = os.path.join(ninja.BIN_DIR, "ninja")
#                     cmake_args += [
#                         "-GNinja",
#                         f"-DCMAKE_MAKE_PROGRAM:FILEPATH={ninja_executable_path}",
#                     ]
#                 except ImportError:
#                     pass
#
#         else:
#
#             # Single config generators are handled "normally"
#             single_config = any(x in cmake_generator for x in {"NMake", "Ninja"})
#
#             # CMake allows an arch-in-generator style for backward compatibility
#             contains_arch = any(x in cmake_generator for x in {"ARM", "Win64"})
#
#             # Specify the arch if using MSVC generator, but only if it doesn't
#             # contain a backward-compatibility arch spec already in the
#             # generator name.
#             if not single_config and not contains_arch:
#                 cmake_args += ["-A", PLAT_TO_CMAKE[self.plat_name]]
#
#             # Multi-config generators have a different way to specify configs
#             if not single_config:
#                 cmake_args += [
#                     f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}"
#                 ]
#                 build_args += ["--config", cfg]
#
#         if sys.platform.startswith("darwin"):
#             # Cross-compile support for macOS - respect ARCHFLAGS if set
#             archs = re.findall(r"-arch (\S+)", os.environ.get("ARCHFLAGS", ""))
#             if archs:
#                 cmake_args += ["-DCMAKE_OSX_ARCHITECTURES={}".format(";".join(archs))]
#
#         # Set CMAKE_BUILD_PARALLEL_LEVEL to control the parallel build level
#         # across all generators.
#         if "CMAKE_BUILD_PARALLEL_LEVEL" not in os.environ:
#             # self.parallel is a Python 3 only way to set parallel jobs by hand
#             # using -j in the build_ext call, not supported by pip or PyPA-build.
#             if hasattr(self, "parallel") and self.parallel:
#                 # CMake 3.12+ only.
#                 build_args += [f"-j{self.parallel}"]
#
#         build_temp = os.path.join(self.build_temp, ext.name)
#         if not os.path.exists(build_temp):
#             os.makedirs(build_temp)
#
#         subprocess.check_call(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)
#         subprocess.check_call(["cmake", "--build", "."] + build_args, cwd=build_temp)
#
#
# # The information here can also be placed in setup.cfg - better separation of
# # logic and declaration, and simpler if you include description/version in a file.
# setup(
#     name="colo_nvme",
#     version="0.0.1",
#     author="Dean Moldovan",
#     author_email="dean0x7d@gmail.com",
#     description="A test project using pybind11 and CMake",
#     long_description="",
#     ext_modules=[CMakeExtension(".")],
#     cmdclass={"build_ext": CMakeBuild},
#     zip_safe=False,
#     # extras_require={"test": ["pytest>=6.0"]},
#     # python_requires=">=3.6",
# )

# import multiprocessing
# import os
# import platform
# import subprocess
# import sys
#
# from setuptools import find_packages
# from setuptools import setup, Extension
# from setuptools.command.build_ext import build_ext
#
#
# class CMakeExtension(Extension):
#     def __init__(self, name, sourcedir=""):
#         # don't invoke the original build_ext for this special extension
#         Extension.__init__(self, name, sources=[])
#         self.sourcedir = os.path.abspath(sourcedir)
#
#
# class CMakeBuild(build_ext):
#     def run(self):
#         for ext in self.extensions:
#             self.build_cmake(ext)
#         super().run()
#
#     def build_cmake(self, ext):
#         extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
#         cmake_args = [
#             # "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={}".format(extdir),
#                       # "-DCMAKE_PREFIX_PATH={}".format(self.pytorch_dir),
#                       # "-DPYTHON_EXECUTABLE:FILEPATH={}".format(self.python_exe),
#                       # "-DCMAKE_CXX_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=0",  # for kenlm - avoid seg fault
#                       # "-DPYTHON_EXECUTABLE=".format(sys.executable),
#                       ]
#
#         config = "Debug" if self.debug else "Release"
#         build_args = ["--config", config]
#         cmake_args += ["-DCMAKE_BUILD_TYPE=" + config]
#
#         if not os.path.exists(self.build_temp):
#             os.makedirs(self.build_temp)
#         cwd = os.getcwd()
#         os.chdir(os.path.dirname(extdir))
#         self.spawn(["cmake", ext.sourcedir] + cmake_args)
#         if not self.dry_run:
#             self.spawn(["cmake", "--build", ".", "--", "-j{}".format(multiprocessing.cpu_count())])
#         os.chdir(cwd)
#
#
# setup(
#     name="colo_nvme",
#     version="0.0.1",
#     description="Library with end-to-end losses and decoders",
#     author="Vladimir Bataev",
#     author_email="artbataev@gmail.com",
#     url="https://artbataev.github.io/end2end/",
#     # packages=find_packages(),
#     # license="MIT",
#     # install_requires=["numpy", "numba", "tqdm"],
#     ext_modules=[CMakeExtension("colo_nvme")],
#     cmdclass={"build_ext": CMakeBuild, }
# )
