FROM fedora:27

RUN dnf install -y \
  boost-devel \
  botan-devel \
  log4cplus-devel \
  gtest-devel \
  postgresql postgresql-devel \
  gcc-c++ \
  automake libtool \
  procps-ng procps \
  iproute net-tools \
  supervisor \
  gtest-devel \
  curl

# get gtest release 1.8 sources
WORKDIR /gtest
RUN curl -OL https://github.com/google/googletest/archive/release-1.8.0.tar.gz
RUN tar xzf release-1.8.0.tar.gz

# build and install kea
WORKDIR /kea
COPY . .
RUN autoreconf -if
RUN ./configure \
  --with-dhcp-pgsql \
  --with-gtest-source=/gtest/googletest-release-1.8.0
RUN make -j`nproc`
RUN make install

CMD ["supervisord", "-n"]
