# Overview
Based on https://github.com/alessandrocarminati/mem_chk_map 
Modified to demo of file/memory deduplication on AutoSD image build.

# Usage
Build a shared libray
```
# build a libwaitinput so and save in _target folder
$ make lib

$ ls _target/
libwaitinput.so
```

Build two container images and copied the shared libary to each images
```
$ make container_images

$ podman images | grep mem_chk_map
localhost/mem_chk_map2                       latest               118abcbd69fa  2 minutes ago   168 MB
localhost/mem_chk_map1                       latest               9856f7a3ce55  2 minutes ago   168 MB
```

Build OS Image
```
# run `sudo rm -rf /var/cache/dnf/autosd*` if any error happens
$ make build_os_image
```

Run OS Image
```
$ make run_os_image

$ sshpass -p password ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p 2222 root@localhost
```

Demo on AutoSD
```
$ gcc -o /root/mem_chk_map1  -D END_WORD=\"end1\" /usr/gu/mem_chk_map.c -L=/usr/gu/_target -l=waitinput

$ LD_LIBRARY_PATH=/usr/gu/_target /root/mem_chk_map1
```

# Useful commands for dev
```
# on natively on dev machine
make lib
make exe
LD_LIBRARY_PATH=./_target ./_target/mem_chk_map1

# run container
sudo podman run --rm -it --privileged mem_chk_map1
sudo podman run --rm -it --privileged mem_chk_map2
```
