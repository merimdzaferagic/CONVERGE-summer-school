# Build args
################
# OS_VERSION            Ubuntu OS version
# UHD_VERSION           UHD version number
# DPDK_VERSION          DPDK version number
# MARCH                 gcc/clang compatible arch
# NUM_JOBS              Number or empty for all
# EXTRA_CMAKE_ARGS      Extra flags for srsRAN Project

ARG OS_VERSION=24.04
ARG MARCH=native
ARG NUM_JOBS=""

##################
# Stage 1: Build #
##################
FROM ubuntu:$OS_VERSION AS builder

# Adding the complete repo to the context, in /src folder
ADD . /src
# An alternative could be to download the repo
# RUN apt update && apt-get install -y --no-install-recommends git git-lfs
# RUN git clone https://github.com/srsran/srsRAN_Project.git /src

# Install srsRAN build dependencies
RUN /src/docker/scripts/install_flexRIC_dependencies.sh build && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends git clang

ARG MARCH
ARG NUM_JOBS

# Compile srsRAN Project and install it in the OS
ARG COMPILER=gcc-10
ARG EXTRA_CMAKE_ARGS=""

WORKDIR /src/docker/flexRIC/flexric/
# RUN git clone https://gitlab.eurecom.fr/mosaic5g/flexric.git 
#WORKDIR /near-RT/flexric
#RUN git checkout br-flexric
WORKDIR /src/docker/flexRIC/flexric/build
#WORKDIR /near-RT/flexric/build
# RUN cd build
RUN cmake -DKPM_VERSION=KPM_V3_00 -DXAPP_DB=NONE_XAPP ../
RUN make
RUN make install

#WORKDIR /near-RT/flexric
WORKDIR /src/docker/flexRIC/flexric/
