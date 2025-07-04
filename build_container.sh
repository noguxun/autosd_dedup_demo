sudo podman build -t mem_chk_map1 --build-arg EXEC=mem_chk_map1 -f Containerfile .
sudo podman build -t mem_chk_map2 --build-arg EXEC=mem_chk_map2 -f Containerfile .
