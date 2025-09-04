ARG DOCKER_IMAGE=alpine:3.19
FROM $DOCKER_IMAGE AS dev

ENV LUAJIT_VERSION v2.1

RUN apk add --no-cache git build-base cmake curl-dev zlib-dev zstd-dev \
		sqlite-dev postgresql-dev hiredis-dev leveldb-dev \
		gmp-dev jsoncpp-dev ninja ca-certificates

WORKDIR /usr/src/
RUN git clone --recursive https://github.com/jupp0r/prometheus-cpp && \
		cd prometheus-cpp && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local \
			-DCMAKE_BUILD_TYPE=Release \
			-DENABLE_TESTING=0 \
			-GNinja && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://github.com/libspatialindex/libspatialindex && \
		cd libspatialindex && \
		cmake -B build \
			-DCMAKE_INSTALL_PREFIX=/usr/local && \
		cmake --build build && \
		cmake --install build && \
	cd /usr/src/ && \
	git clone --recursive https://luajit.org/git/luajit.git -b ${LUAJIT_VERSION} && \
		cd luajit && \
		make amalg && make install && \
	cd /usr/src/

FROM dev as builder

COPY .git /usr/src/eclipse/.git
COPY CMakeLists.txt /usr/src/eclipse/CMakeLists.txt
COPY README.md /usr/src/eclipse/README.md
COPY minetest.conf.example /usr/src/eclipse/minetest.conf.example
COPY builtin /usr/src/eclipse/builtin
COPY cmake /usr/src/eclipse/cmake
COPY doc /usr/src/eclipse/doc
COPY fonts /usr/src/eclipse/fonts
COPY lib /usr/src/eclipse/lib
COPY misc /usr/src/eclipse/misc
COPY po /usr/src/eclipse/po
COPY src /usr/src/eclipse/src
COPY irr /usr/src/eclipse/irr
COPY textures /usr/src/eclipse/textures

WORKDIR /usr/src/eclipse
RUN cmake -B build \
		-DCMAKE_INSTALL_PREFIX=/usr/local \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_SERVER=TRUE \
		-DENABLE_PROMETHEUS=TRUE \
		-DBUILD_UNITTESTS=FALSE -DBUILD_BENCHMARKS=FALSE \
		-DBUILD_CLIENT=FALSE \
		-GNinja && \
	cmake --build build && \
	cmake --install build

FROM $DOCKER_IMAGE AS runtime

RUN apk add --no-cache curl gmp libstdc++ libgcc libpq jsoncpp zstd-libs \
				sqlite-libs postgresql hiredis leveldb && \
	adduser -D minetest --uid 30000 -h /var/lib/minetest && \
	chown -R minetest:minetest /var/lib/minetest

WORKDIR /var/lib/minetest

COPY --from=builder /usr/local/share/eclipse /usr/local/share/eclipse
COPY --from=builder /usr/local/bin/eclipseserver /usr/local/bin/eclipseserver
COPY --from=builder /usr/local/share/doc/eclipse/minetest.conf.example /etc/minetest/minetest.conf
COPY --from=builder /usr/local/lib/libspatialindex* /usr/local/lib/
COPY --from=builder /usr/local/lib/libluajit* /usr/local/lib/
USER minetest:minetest

EXPOSE 30000/udp 30000/tcp
VOLUME /var/lib/minetest/ /etc/minetest/

ENTRYPOINT ["/usr/local/bin/eclipseserver"]
CMD ["--config", "/etc/minetest/minetest.conf"]
