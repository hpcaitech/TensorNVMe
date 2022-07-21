FROM hpcaitech/pytorch-cuda:1.11.0-11.3.0

# install dependencies
RUN conda install -y cmake

# install tensornvme
RUN git clone https://github.com/hpcaitech/TensorNVMe.git && \
    cd TensorNVMe && \
    pip install -r requirements.txt && \
    pip install -v --no-cache-dir .