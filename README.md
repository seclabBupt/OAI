# OAI配置流程
## 软件安装
安装Xshell（可以直接在官网下载安装）
连接服务器：安装好之后进去新建会话，输入服务器地址、用户名、密码，就可以连接成功了

Xftp，可用于将代码拷到本地方便阅读，连接服务器的步骤与Xshell类似


## 内核相关处理
考虑到OAI 对内核非常敏感，很多莫名其表的错误都是由内核不适应导致的，所以安装一下低延时内核等模块

### 安装 low-latency kernel（低延时内核）：
sudo apt-get install linux-lowlatency
sudo apt-get install linux-image-`uname -r | cut -d- -f1-2`-lowlatency
sudo apt-get install linux-headers-`uname -r | cut -d- -f1-2`-lowlatency
sudo reboot

### 加载 GTP 内核模块（for OAI-CN）：
sudo modprobe gtp
dmesg | tail # You should see something that says about GTP kernel module
为了让OAI支持接入更多的UE，可能会需要修改CPU相关功能来压榨PC的性能，具体涉及到 在 BIOS 中移除电源管理功能（P-states, C-states）、在 BIOS 中关闭超线程（hyper-threading）、禁用 Intel CPU 的 P-state 驱动（Intel CPU 专用的频率调节器驱动）、将 intel_powerclamp（Intel 电源管理驱动程序）加入启动黑名单、关闭 CPU 睿频，这里暂不使用，需要的话查看https://www.cnblogs.com/jmilkfan-fanguiju/p/12789792.html

## OAI核心网部署
来源：https://gitlab.eurecom.fr/mosaic5g/mosaic5g/-/wikis/tutorials/oai-cn
//Install OAI-CN as a snap:
sudo snap install oai-cn --channel=edge --devmode

//Check the installation:
sudo oai-cn.help 
核心网需要MySQL注册UE列表，所以先安装mysql：

### mysql安装
$ sudo apt install mysql-server mysql-client
$ sudo mysql_secure_installation # after new installation #root用户先暂停这一步
$ mysql -u root -p
注：在第二步中如果是作为root用户,则需要提前设置密码，步骤如下：
sudo mysql  #进入mysql命令行
ALTER USER 'root'@'localhost' IDENTIFIED WITH mysql_native_password by 'mynewpassword';  #mynewpassword改为自己的密码
exit
sudo mysql_secure_installation #回归之前暂停的第二步

### HSS安装部署
1.	初始化 HSS: sudo oai-cn.hss-init
2.	找到 configuration 文件: sudo oai-cn.hss-conf-get，屏幕会输出该文件所在的路径
3.	修改 hss.conf 文件，修改登录mysql的用户名（root）和密码（之前自己设置的密码），并修改OPERATOR_key = "11111111111111111111111111111111";并且该文件内会有hss_fd.conf的路径
4.	修改 hss_fd.conf, 检查 Identity 来匹配 .openair4G.eur ，在命令行查看/etc/hosts文件，可以看到hss和mme有相应的identity。
5.	建立证书：oai-cn.hss-init
6.	运行HSS： sudo oai-cn.hss
7.	最后一行为"Initializing S6a layer: DONE"即部署成功

### MME安装部署
1.	初始化 MME: sudo oai-cn.mme-init
2.	找到 configuration 文件: sudo oai-cn.mme-conf-get
3.	在 mme.conf: 修改并记录GUMMEI_LIST和TAI_LIST的前两个参数保持一致（两个连起来即PLMN），此外还需要检查NETWORK_INTERFACES: MME_IPV4_ADDRESS_FOR_S1_MME to 127.0.1.10/24、MME_IPV4_ADDRESS_FOR_S11_MME to 127.0.11.1/8、S-GW: SGW_IPV4_ADDRESS_FOR_S11 to 127.0.11.2/8，但是现在版本应该默认就是对的不需要修改，在该文件中找到mme_fd.conf的路径
4.	在 mme_fd.conf: 检查identity 和 connect peer的hostname，但默认也是对的
5.	启动MME：sudo oai-cn.mme
6.	最后一行为Peer .openair4G.eur is now connected即成功（保持HSS后台运行）

### SPGW安装部署
1.	初始化 SPGW: sudo oai-cn.spgw-init
2.	在 spgw.conf文件中，修改以下参数
SGW_IPV4_ADDRESS_FOR_S11 to 127.0.11.2/8
SGW_IPV4_ADDRESS_FOR_S1U_S12_S4_UP to 127.0.1.10/24
PGW_INTERFACE_NAME_FOR_SGI: the interface to the Internet
DEFAULT_DNS_IPV4_ADDRESS: your DNS
3.	启动 SPGW: sudo oai-cn.spgw
4.	最后一行为 Initializing SPGW-APP task interface: DONE 即成功



## 启动传输步骤
基站和UE需要各自打开一个窗口，都先进入该路径：cd oai/openairinterface5g-2021.w23/cmake_targets/ran_build/build

一个窗口启动基站：RFSIMULATOR=server ./nr-softmodem -O ../../../targets/PROJECTS/GENERIC-LTE-EPC/CONF/gnb.band78.tm1.106PRB.usrpn300.conf --parallel-config PARALLEL_SINGLE_THREAD --rfsim --phy-test --noS1 --nokrnmod 1
另一个窗口启动UE：RFSIMULATOR=127.0.0.1 ./nr-uesoftmodem --rfsim --phy-test
基站和UE便连接成功了，会有输出信息


对代码内容进行修改之后需要重新编译：
都先进入该路径 cd oai/openairinterface5g-2021.w23/cmake_targets/ran_build/build
编译基站：make -j$(nproc) nr-softmodem
编译UE： make -j$(nproc) nr-uesoftmodem
编译完成后就可以启动基站和UE了

## DMRS输出步骤
首先在路径openairinterface5g-2021.w23\openair1\PHY\NR_UE_ESTIMATION\nr_dl_channel_estimation.c下添加printf输出语句:
printf("原始的DMRS内容是 %d + %d j, 接收到的DMRS内容是 %d + %d j \n", pil[0],pil[1],rxF[0],rxF[1]);
 
pil[0] 和 pil[1]：基站端DMRS符号的实部和虚部
rxF[0] 和 rxF[1]：实际接收到的DMRS的实部和虚部
//ch[0] 和 ch[1]：对应CSI的实部和虚部
然后按上面的步骤重新编译代码并启动基站和UE
