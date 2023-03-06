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

# 物理层参数
## 1.配置环境
### 1.1 下载并安装VirtualBox
https://www.virtualbox.org/wiki/Downloads  
![image](https://user-images.githubusercontent.com/86646728/221450006-7446ffa8-3bd3-4fa4-b127-3deb6ab26a21.png)

### 1.1 下载Vagrant
https://github.com/mobile-insight/mobileinsight-dev  
git clone https://github.com/mobile-insight/mobileinsight-dev.git/path/to/dev  
cd /path/to/dev  
![image](https://user-images.githubusercontent.com/86646728/221449133-6c2e8a0c-3186-457f-becd-a4790a8dc94e.png)

### 1.2 配置容器
https://github.com/mobile-insight/mobileinsight-dev#customize-mobileinsight

vagrant destroy  
vagrant up  
vagrant ssh  
![image](https://user-images.githubusercontent.com/86646728/221449342-01e99f42-7747-4bf1-9645-11609cbacadd.png)

cd mi-dev  
![image](https://user-images.githubusercontent.com/86646728/221449378-b1a3f5b4-6c84-4d2a-ae00-edf7f41c52b7.png)

## 2. mi2log转txt
https://github.com/mobile-insight/mobileinsight-dev#modify-mobileinsight-core-codes  
cd ~/mi-dev/mobileinsight-core./install-ubuntu.sh 

测试python文件  
python3 xxx.py  

example文件  
https://github.com/mobile-insight/mobileinsight-core/blob/master/examples/offline-analysis-example.py  
![image](https://user-images.githubusercontent.com/86646728/221449539-accea81a-74b6-4f14-8f90-64a827dc174f.png)

运行成功  
![image](https://user-images.githubusercontent.com/86646728/221449576-cfa4406c-a01c-49a1-9923-c1c77246439d.png)
## 3. 传输到本地
### VirtualBox安装Lunix系统ip地址的问题
原ip地址：10.0.2.15  
原因是虚拟机选择的是nat联网方式，将主机虚拟成10.0.2.1然后随机给虚拟机一个10.0.2.15的地址，可
以看网关是多少，那就是虚拟出来主机的地址。  
![image](https://user-images.githubusercontent.com/86646728/221449696-42cd681e-02ef-4837-ba57-e624dc5802ae.png)
![image](https://user-images.githubusercontent.com/86646728/221449727-2c482c5a-6745-4eeb-b0b5-f717af34cdf5.png)  
用户名和密码均为vagrant  
![aee1aeb5670bb07fb1c0c15015cf8d7](https://user-images.githubusercontent.com/86646728/222389644-0c371bc2-0d54-442f-a502-0bc6785a8335.jpg)

# OTFS调制修改代码
## 代码文件介绍：
CMakeLists.txt，该文件为OAI的编译文件，只有当需要编译如fftw的外部函数库时才需要修改该文件，在修改时注意加入函数库位置，越往后的函数库编译优先级越高，所以应当把其他包所依赖的函数包放在最后面，例如fftw就应当放在PHY相关编译的后面。

otfs.c、otfs.h，OTFS调制与解调的头文件和实现文件。

nr_dlsch.c：PDSCH实现文件，由于我的工作是对实现该信道的OTFS调制，因此基站端所有针对OTFS的改动都在这个文件上。

phy_procedures_nr_ue.c：UE端对OTFS解调与信道估计的改动都在这个文件上。

nr-ue.c：这个是UE运行的主文件，在我这里作为了采集UE端数据的代码文件，这个根据自己所需要采集的数据进行修改。根据实验的需要也可以在其他文件来收集数据，需要注意数据流在OAI架构中的变化才能收集到自己所需要的数据。

softmodem-common.c、softmodem-common.h：这两个文件主要是修改命令行的文件，修改好后可以通过命令行设置收发双方是否使用OTFS调制还是原有的OFDM调制，但我遇到了问题并没有实现，运行OTFS调制的话需要将softmodem-common.c中的otfs变量置为1，然后编译基站端和UE端才可以。如果要运行回原本的OFDM的话则需要改回0，并再次编译基站和UE端。

sim.h、random_channel.c、rfsim.c：这两个文件都涉及到信道建模，对于任何类型的信道的description都需要先在sim.h声明，再在random_channel.c中赋予参数，这部分可以学习博客https://blog.csdn.net/BUPTOctopus/article/details/106920535.
参数赋予完成后还应当根据自己所需要的信道性质在rfsim.c进行中进一步操作，例如雷达回波就需要经历两次我描述的信道并且添加移动反射面的路径（时延、多普勒、反射系数）
## fftw包的安装与应用
注意：为了实现OTFS调制我在OAI中引入了一个外部函数库fftw，所以运行OTFS代码时需要已安装解压fftw并且将静态链接添加到OAI的编译中
fftw的官方网址：https://www.fftw.org/download.html.
具体安装应用流程会根据系统而有区别
