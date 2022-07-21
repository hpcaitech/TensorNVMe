import os
import re
import sys
from setuptools import setup, find_packages
from subprocess import call
from typing import List
from platform import uname
from packaging import version


TENSORNVME_INITIALIZE_RE_BLOCK = (
    r"^# >>> tensornvme initialize >>>(?:\n|\r\n)"
    r"([\s\S]*?)"
    r"# <<< tensornvme initialize <<<(?:\n|\r\n)?")


def check_uring_compatibility():
    uname_info = uname()
    if uname_info.system != 'Linux':
        raise RuntimeError('Only Linux is supported')
    kernel_version = version.parse(uname_info.release.split('-')[0])
    return kernel_version >= version.parse('5.10')


this_dir = os.path.dirname(os.path.abspath(__file__))
backend_install_dir = os.path.join(os.path.expanduser('~'), '.tensornvme')

enable_uring = True
enable_aio = True
if os.environ.get('DISABLE_URING') == '1' or not check_uring_compatibility():
    enable_uring = False
if os.environ.get('DISABLE_AIO') == '1':
    enable_aio = False
assert enable_aio or enable_uring
if os.environ.get('WITH_ROOT') == '1':
    backend_install_dir = '/usr'
    if not os.access(backend_install_dir, os.W_OK):
        raise RuntimeError(
            'Permission denied, please make sure you have root access')

libraries = ['aio']
sources = ['csrc/offload.cpp', 'csrc/uring.cpp',
           'csrc/aio.cpp', 'csrc/space_mgr.cpp']
extra_objects = []
define_macros = []
ext_modules = []
cmdclass = {}


def cpp_ext_helper(name, sources, **kwargs):
    from torch.utils.cpp_extension import CppExtension
    extra_include_dirs = []
    if 'C_INCLUDE_PATH' in os.environ:
        extra_include_dirs.extend(os.environ['C_INCLUDE_PATH'].split(':'))
    if 'CPLUS_INCLUDE_PATH' in os.environ:
        extra_include_dirs.extend(os.environ['CPLUS_INCLUDE_PATH'].split(':'))
    extra_include_dirs = list(
        filter(lambda s: len(s) > 0, set(extra_include_dirs)))
    return CppExtension(
        name,
        [os.path.join(this_dir, path) for path in sources],
        include_dirs=[os.path.join(this_dir, 'csrc'), os.path.join(this_dir, 'include'),
                      os.path.join(backend_install_dir, 'include'),
                      *extra_include_dirs],
        library_dirs=[os.path.join(backend_install_dir, 'lib')],
        **kwargs
    )


def find_static_lib(lib_name: str, lib_paths: List[str] = []) -> str:
    static_lib_name = f'lib{lib_name}.a'
    lib_paths.extend(['/usr/lib', '/usr/lib64'])
    if os.environ.get('LIBRARY_PATH', None) is not None:
        lib_paths.extend(os.environ['LIBRARY_PATH'].split(':'))
    for lib_dir in lib_paths:
        if os.path.isdir(lib_dir):
            for filename in os.listdir(lib_dir):
                if filename == static_lib_name:
                    return os.path.join(lib_dir, static_lib_name)
    raise RuntimeError(f'{static_lib_name} is not found in {lib_paths}')


def setup_bachrc():
    init_rules = f'export LD_LIBRARY_PATH="{backend_install_dir}/lib:$LD_LIBRARY_PATH"'
    bachrc_path = os.path.join(os.path.expanduser('~'), '.bashrc')
    with open(bachrc_path, 'a+') as f:
        f.seek(0)
        rules = f.read()
        if not re.search(TENSORNVME_INITIALIZE_RE_BLOCK, rules, flags=re.DOTALL | re.MULTILINE):
            f.write(
                f'# >>> tensornvme initialize >>>\n{init_rules}\n# <<< tensornvme initialize <<<\n')
            print(f'{bachrc_path} is changed, please source it.')


def setup_dependencies():
    build_dir = os.path.join(this_dir, 'cmake-build')
    if not enable_uring:
        define_macros.append(('DISABLE_URING', None))
        sources.remove('csrc/uring.cpp')
    if not enable_aio:
        define_macros.append(('DISABLE_AIO', None))
        sources.remove('csrc/aio.cpp')
        libraries.remove('aio')
    os.makedirs(build_dir, exist_ok=True)
    os.makedirs(backend_install_dir, exist_ok=True)
    os.chdir(build_dir)
    call(['cmake', '..', f'-DBACKEND_INSTALL_PREFIX={backend_install_dir}'])
    if enable_uring:
        call(['make', 'extern_uring'])
        extra_objects.append(find_static_lib(
            'uring', [os.path.join(backend_install_dir, 'lib')]))
    if enable_aio:
        call(['make', 'extern_aio'])
    os.chdir(this_dir)
    if os.environ.get('WITH_ROOT') != '1':
        setup_bachrc()


if sys.argv[1] in ('install', 'develop', 'bdist_wheel'):
    setup_dependencies()
    from torch.utils.cpp_extension import BuildExtension
    ext_modules.append(cpp_ext_helper('tensornvme._C', sources,
                                      extra_objects=extra_objects,
                                      libraries=libraries,
                                      define_macros=define_macros
                                      ))
    cmdclass['build_ext'] = BuildExtension


def get_version():
    with open('version.txt') as f:
        version = f.read().strip()
        return version


def fetch_requirements(path):
    with open(path, 'r') as fd:
        return [r.strip() for r in fd.readlines()]


def fetch_readme():
    with open('README.md', encoding='utf-8') as f:
        return f.read()


setup(
    name='tensornvme',
    version=get_version(),
    packages=find_packages(exclude=(
        '3rd',
        'csrc',
        'tests',
        'include',
        '*.egg-info'
    )),
    ext_modules=ext_modules,
    cmdclass=cmdclass,
    entry_points={
        'console_scripts': ['tensornvme=tensornvme.cli:cli']
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
