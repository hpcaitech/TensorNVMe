import os
import sys
import torch
from setuptools import setup, find_packages
from torch.utils.cpp_extension import CppExtension, BuildExtension
from subprocess import call
from typing import List
from platform import uname
from packaging import version


def check_uring_compatibility():
    uname_info = uname()
    if uname_info.system != 'Linux':
        raise RuntimeError('Only Linux is supported')
    kernel_version = version.parse(uname_info.release.split('-')[0])
    return kernel_version >= version.parse('5.10')


this_dir = os.path.dirname(os.path.abspath(__file__))
enable_uring = True
enable_aio = True
if os.environ.get('DISABLE_URING') == '1' or not check_uring_compatibility():
    enable_uring = False
if os.environ.get('DISABLE_AIO') == '1':
    enable_aio = False

extra_compile_args = []
libraries = ['aio']
sources = ['csrc/offload.cpp', 'csrc/uring.cpp',
           'csrc/aio.cpp', 'csrc/space_mgr.cpp']
extra_objects = []


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


def setup_dependencies():
    build_dir = os.path.join(this_dir, 'cmake-build')
    if not enable_uring:
        extra_compile_args.append('-DDISABLE_URING')
        sources.remove('csrc/uring.cpp')
    if not enable_aio:
        extra_compile_args.append('-DDISABLE_AIO')
        sources.remove('csrc/aio.cpp')
        libraries.remove('aio')
    os.makedirs(build_dir, exist_ok=True)
    os.chdir(build_dir)
    call(['cmake', '..'])
    if enable_uring:
        call(['make', 'extern_uring'])
        extra_objects.append(find_static_lib(
            'uring', [os.path.join(build_dir, '3rd/lib')]))
    if enable_aio:
        call(['make', 'extern_aio'])
    os.chdir(this_dir)


if sys.argv[1] in ('install', 'develop', 'bdist_wheel'):
    setup_dependencies()


def get_version():
    with open('version.txt') as f:
        version = f.read().strip()
        torch_version = '.'.join(torch.__version__.split('.')[:2])
        version += f'+torch{torch_version}'
        return version


def fetch_requirements(path):
    with open(path, 'r') as fd:
        return [r.strip() for r in fd.readlines()]


def fetch_readme():
    with open('README.md', encoding='utf-8') as f:
        return f.read()


setup(
    name='colo_nvme',
    version=get_version(),
    packages=find_packages(exclude=(
        '3rd',
        'csrc',
        'tests',
        'include',
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
    },
    description='A tensor disk offloader without data copying.',
    long_description=fetch_readme(),
    long_description_content_type='text/markdown',
    license='Apache Software License 2.0',
    install_requires=fetch_requirements('requirements.txt'),
    python_requires='>=3.6',
    classifiers=[
        'Programming Language :: Python :: 3',
        'License :: OSI Approved :: Apache Software License',
        'Topic :: Scientific/Engineering :: Artificial Intelligence',
    ],
)
