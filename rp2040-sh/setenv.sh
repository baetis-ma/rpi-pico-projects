#export PICO_SDK_PATH=~/rpi-pico/pico-sdk/
export PICO_SDK_PATH=/opt/pico-sdk
export PICO_TOOLCHAIN_PATH=/opt/gcc-arm-none-eabi
echo "DID YOU SOURCE THIS EXECUTION?"
echo 'mkdir build; cd build; cmake -DPICO_BOARD=pico_w ..;  make'
env | grep SDK
env | grep TOOLCHAIN
