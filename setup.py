import os
from setuptools import setup, find_packages
from torch.utils.cpp_extension import CppExtension, BuildExtension
from subprocess import call
import sys
from typing import List


this_dir = os.path.dirname(os.path.abspath(__file__))


def cpp_ext_helper(name, sources, **kwargs):
    return CppExtension(
        name,
        [os.path.join(this_dir, path) for path in sources],
        include_dirs=[os.path.join(this_dir, 'csrc'), os.path.join(this_dir, 'include'),
                      os.path.join(this_dir, 'cmake-build/3rd/include')],
        library_dirs=[os.path.join(this_dir, 'cmake-build/3rd/lib')],
        **kwargs
    )


def find_static_lib(lib_name: str, lib_paths: List[str] = []) -> str:
    static_lib_name = f'lib{lib_name}.a'
    lib_paths.extend(['/usr/lib', '/usr/lib64', '/usr/local/lib',
                      '/usr/local/lib64'])
    if os.environ.get('LIBRARY_PATH', None) is not None:
        lib_paths.extend(os.environ['LIBRARY_PATH'].split(':'))
    for lib_dir in lib_paths:
        if os.path.isdir(lib_dir):
            for filename in os.listdir(lib_dir):
                if filename == static_lib_name:
                    return os.path.join(lib_dir, static_lib_name)
    raise RuntimeError(f'{static_lib_name} is not found in {lib_paths}')


def install_dependencies():
    build_dir = os.path.join(this_dir, 'cmake-build')
    os.makedirs(build_dir, exist_ok=True)
    os.chdir(build_dir)
    call(['cmake', '..'])
    call(['make'])
    os.chdir(this_dir)


extra_compile_args = []
libraries = ['aio']
sources = ['csrc/offload.cpp', 'csrc/uring.cpp',
           'csrc/aio.cpp', 'csrc/space_mgr.cpp']
extra_objects = []
if os.environ.get('DISABLE_URING') == '1':
    extra_compile_args.append('-DDISABLE_URING')
    sources.remove('csrc/uring.cpp')
if os.environ.get('DISABLE_AIO') == '1':
    extra_compile_args.append('-DDISABLE_AIO')
    libraries.remove('aio')
    sources.remove('csrc/aio.cpp')


if sys.argv[1] in ('install', 'develop'):
    install_dependencies()
    if os.environ.get('DISABLE_URING') != '1':
        extra_objects.append(find_static_lib(
            'uring', [os.path.join(this_dir, 'cmake-build/3rd/lib')]))


setup(
    name='colo_nvme',
    packages=find_packages(exclude=(
        'csrc',
        'tests',
        '*.egg-info'
    )),
    ext_modules=[cpp_ext_helper('colo_nvme._C', sources,
                                extra_compile_args=extra_compile_args,
                                extra_objects=extra_objects,
                                libraries=libraries
                                )],
    cmdclass={'build_ext': BuildExtension},
    entry_points={
        'console_scripts': ['colonvme=colo_nvme.cli:cli']
    }
)
