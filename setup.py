import os
import pathlib

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sources=None, sourcedir=''):
        if sources is None:
            sources = []
        Extension.__init__(self, name, sources=sources)
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
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
        self.spawn(["cmake", f"{str(extdir.parent.absolute())}"] + cmake_args)
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
    ext_modules=[CMakeExtension('colo_nvme')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
