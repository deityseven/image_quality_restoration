1. 安装docker
2. 下载基础镜像 http://download.openvz.org/template/precreated/centos-7-x86_64.tar.gz
3. docker载入镜像 docker import centos-7-x86_64.tar.gz centos7_64:none
3. 管理员模式启动容器，挂载光驱，挂载显卡 docker run -it --user="root" --privileged --cpus=15 --publish-all=true --name="develop_image_quality_restoration" --net="host" -v G:/:/dvd centos7_64:none /bin/bash
4. 安装gcc