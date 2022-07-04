from setuptools import setup
from torch.utils.cpp_extension import CppExtension, BuildExtension

setup(
    name='colo_nvme',
    ext_modules=[CppExtension('colo_nvme', ['csrc/offload.cpp', 'csrc/uring.cpp', 'csrc/space_mgr.cpp'],
                              include_dirs=['csrc'],
                              extra_compile_args=['-luring'],
                              libraries=['uring'])],
    # cmdclass={'build_ext': BuildExtension}
)
