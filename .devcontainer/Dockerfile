FROM mcr.microsoft.com/azurespheresdk:latest AS dev

FROM dev AS build
COPY ./device /src/
WORKDIR /out
RUN cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE="/opt/azurespheresdk/CMakeFiles/AzureSphereToolchain.cmake" \
    -DAZURE_SPHERE_TARGET_API_SET="latest-lts" -DCMAKE_BUILD_TYPE="Release" -DSE_BUILD_LOCAL=OFF -DSE_BUILD_TYPE="Tests" -DSE_BUILD_M4=OFF "/src"
ENTRYPOINT [ "ninja" ]
