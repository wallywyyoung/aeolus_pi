IMAGE_NAME = aeolus-builder
DOCKER_RUN = docker run --rm -v $(PWD):/workspace -v ~/.cache/vcpkg:/root/.cache/vcpkg $(IMAGE_NAME)

.PHONY: all image shell clean

all: image
	$(DOCKER_RUN) /bin/bash -c "cd buildroot && make BR2_EXTERNAL=../buildroot-ext raspberrypi4_64_defconfig && make"

image:
	docker build -t $(IMAGE_NAME) .

shell: image
	$(DOCKER_RUN) /bin/bash

clean:
	$(DOCKER_RUN) /bin/bash -c "cd buildroot && make clean"
