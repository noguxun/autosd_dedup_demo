all: lib exe

lib:
	# using a Fedora 42 based container to build the shared library
	mkdir -p _target/
	podman build -t libwaitinput-builder -f Containerfile.builder .
	podman run --name temp-builder libwaitinput-builder

	# copy the compiled so file from the container to the host
	podman cp temp-builder:/build/_target/libwaitinput.so _target/
	podman rm temp-builder

container_images:
	TAG_NAME=mem_chk_map1; sudo podman build -t $$TAG_NAME --build-arg EXEC=$$TAG_NAME -f Containerfile .
	TAG_NAME=mem_chk_map2; sudo podman build -t $$TAG_NAME --build-arg EXEC=$$TAG_NAME -f Containerfile .

build_os_image:
	# Trace execution & Exit on error
	set -xe
	automotive-image-builder --verbose \
	    build \
	    --distro autosd9 \
	    --target qemu \
	    --mode image \
	    --build-dir=_build \
	    --export qcow2 \
	    dedup_demo.aib.yml \
	    _target/dedup_demo.$$(arch).img.qcow2

run_os_image:
	automotive-image-runner --nographics _target/dedup_demo.$$(arch).img.qcow2

exe: mem_chk_map.c 
	mkdir -p _target/
	gcc $(CFLAGS) -o _target/mem_chk_map1  -D END_WORD=\"end1\" mem_chk_map.c -L=_target -l=waitinput
	gcc $(CFLAGS) -o _target/mem_chk_map2  -D END_WORD=\"end2\" mem_chk_map.c -L=_target -l=waitinput

clean:
	sudo rm -rf _build
	rm -rf _target
	podman rm -f temp-builder
	podman rmi -f libwaitinput-builder
	dedup_demo.$$(arch).img.qcow2
