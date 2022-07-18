import os
from setuptools import setup, find_packages
from torch.utils.cpp_extension import CppExtension, BuildExtension

this_dir = os.path.dirname(os.path.abspath(__file__))


def cpp_ext_helper(name, sources, **kwargs):
    return CppExtension(
        name,
        [os.path.join(this_dir, path) for path in sources],
        include_dirs=[os.path.join(this_dir, 'csrc'), os.path.join(this_dir, 'include')],
        **kwargs
    )


extra_compile_args = []
libraries = ['uring', 'aio']
if os.environ.get('DISABLE_URING') == '1':
    extra_compile_args.append('-DDISABLE_URING')
    libraries.remove('uring')
if os.environ.get('DISABLE_AIO') == '1':
    extra_compile_args.append('-DDISABLE_AIO')
    libraries.remove('aio')

setup(
    name='colo_nvme',
    packages=find_packages(exclude=(
        'csrc',
        'tests',
        '*.egg-info'
    )),
    ext_modules=[cpp_ext_helper('colo_nvme._C', ['csrc/offload.cpp', 'csrc/uring.cpp', 'csrc/aio.cpp', 'csrc/space_mgr.cpp'],
                                extra_compile_args=extra_compile_args,
                                libraries=libraries)],
    cmdclass={'build_ext': BuildExtension}
)
