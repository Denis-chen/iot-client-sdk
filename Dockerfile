FROM ubuntu
RUN apt-get update && apt-get install -y \ 
  gcc \
  g++ \
  gcc-5-multilib \
  g++-5-multilib \
  gcc-5-arm-linux-gnueabi \ 
  g++-5-arm-linux-gnueabi \
  gcc-5-arm-linux-gnueabihf \ 
  g++-5-arm-linux-gnueabihf \
  make \
  && ln -s /usr/include/asm-generic /usr/include/asm 

WORKDIR /src/
