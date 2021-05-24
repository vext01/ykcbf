#! /bin/sh

set -e

make bf_base

export CARGO_HOME="`pwd`/.cargo"
export RUSTUP_HOME="`pwd`/.rustup"

curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs > rustup.sh
sh rustup.sh --default-host x86_64-unknown-linux-gnu \
    --default-toolchain nightly \
    --no-modify-path \
    --profile minimal \
    -y
export PATH=`pwd`/.cargo/bin/:$PATH

rustup toolchain install nightly

git clone https://github.com/ykjit/ykllvm
cd ykllvm
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../inst \
    -DLLVM_INSTALL_UTILS=On \
    -DCMAKE_BUILD_TYPE=release \
    -DLLVM_ENABLE_ASSERTIONS=On \
    -DLLVM_ENABLE_PROJECTS="lld;clang" \
    ../llvm
make -j `nproc` install
export PATH=`pwd`/../inst/bin:${PATH}
cd ../..

git clone https://github.com/softdevteam/yk/
cd yk && cargo build
YK_INST_DIR=`pwd`/target/debug/
cd ..

LDFLAGS="-L$YK_INST_DIR -Wl,-rpath=$YK_INST_DIR" \
  CPPFLAGS=-I`pwd`/yk/ykcapi \
  make bf_simple_yk

cd lang_tests && cargo test
