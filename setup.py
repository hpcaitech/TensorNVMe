from setuptools import setup, find_packages
from torch.utils.cpp_extension import CppExtension, BuildExtension

setup(
    name='colo_nvme',
    packages=find_packages(exclude=(
        'csrc',
        'tests',
        '*.egg-info'
    )),
    ext_modules=[CppExtension('colo_nvme._C', ['csrc/offload.cpp', 'csrc/uring.cpp', 'csrc/aio.cpp', 'csrc/space_mgr.cpp'],
                              include_dirs=['csrc', 'csrc/include'],
                              extra_compile_args=['-luring', '-aio'],
                              libraries=['uring', 'aio'])],
    cmdclass={'build_ext': BuildExtension}
)
