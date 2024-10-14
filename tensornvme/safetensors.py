import torch
import json

from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict

_float8_e4m3fn = getattr(torch, "float8_e4m3fn", None)
_float8_e5m2 = getattr(torch, "float8_e5m2", None)

_TYPES = {
    torch.float64: "F64",
    torch.float32: "F32",
    torch.float16: "F16",
    torch.bfloat16: "BF16",
    torch.int64: "I64",
    torch.int32: "I32",
    torch.int16: "I16",
    torch.int8: "I8",
    torch.uint8: "U8",
    torch.bool: "BOOL",
    _float8_e4m3fn: "F8_E4M3",
    _float8_e5m2: "F8_E5M2"
}

@dataclass
class TensorInfo:
    dtype: str
    shape: List[int]
    data_offsets: Tuple[int, int]

@dataclass
class PreparedData:
    n: int
    header_bytes: bytes
    offset: int

def prepare(data: Dict[str, torch.Tensor], meta_info: Optional[Dict[str, str]]) -> Tuple[PreparedData, List[torch.Tensor]]:
    sorted_data = sorted(data.items(), key=lambda x: (x[1].dtype, x[0]))

    tensors = []
    metadata = {}
    offset = 0

    for name, tensor in sorted_data:
        n = tensor.numel() * tensor.element_size()
        tensor_info = TensorInfo(dtype=_TYPES[tensor.dtype], shape=list(tensor.shape), data_offsets=(offset, offset + n))
        offset += n
        metadata[name] = asdict(tensor_info)
        tensors.append(tensor)

    metadata_buf = json.dumps(metadata).encode('utf-8')

    extra = (8 - len(metadata_buf) % 8) % 8
    metadata_buf += b' ' * extra

    n = len(metadata_buf)

    return PreparedData(n=n, header_bytes=metadata_buf, offset=offset), tensors
