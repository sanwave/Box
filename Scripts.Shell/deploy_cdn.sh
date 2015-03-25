#!/bin/bash

#执行前建议修改参数，防止端口冲突
home_dir=/mpeg
ftp_user_pswd=cdn_r007

ci_port=18110
rti_port=18160
cls_port=18188
redis_port=18379
cpm_port=18186
cl_port=18130
cl_sport=38130
cg_port=18230
cg_sport=38230
cdnadapter_port=18270
rtcl_port=18150

local_ip=$1
cls_ip=${local_ip}
cpm_ip=${local_ip}
ci_ip=${local_ip}
cl_ip=${local_ip}
cg_ip=${local_ip}
redis_ip=${local_ip}
rti_ip=${local_ip}
rtcl_ip=${local_ip}
db_ip=$2
db_name=$3
db_user=root
db_pswd=coship
db_port=3306

#以下变量为默认参数不需要修改
ci_old_ip=172.20.15.7
ci_old_port=8010
cpm_old_ip=172.20.15.10
cpm_old_port=8086
db_old_ip=172.20.15.11
db_old_name=cpmdb
redis_old_addr=172.20.15.1:6379
cls_old_ip=172.20.15.7
cls_old_port=8088
cl_old_addr=172.20.15.7:8030
cl_old_saddr=172.20.15.7:28030
cl_old_ip=172.20.15.7
rti_old_ip=172.30.54.89
rti_old_port=8060
rtcl_old_ip=172.20.15.10
rtcl_old_port=8050

current_dir=`pwd`
redhat_ver=`uname -a|awk -F "el" '{print $2;}'|awk -F " #" '{print $1;}'`

function check_para()
{

	if [ "${local_ip}"x = x -o "${db_ip}"x = x -o "${db_name}"x = x ]; then
		echo
		echo "Usage: deploy_cdn.sh <local_ip> <db_ip> <db_name>"
		echo "e.g. ./deploy_cdn.sh 172.21.11.169 172.21.11.169 cpmdb_b031"
		exit
	fi
	
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "show databases;"|grep -w ${db_name} >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo
		echo "数据库服务器已存在同名数据库！请重新配置部署数据库名或手动移除该数据库"
		echo "脚本即将退出"
		exit
	fi
}





function unzip()
{
	echo "=====  start unzip!  ====="
	echo
	
	ls *.tar.gz >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo 'unpacking...'
		ls *.tar.gz | xargs -n1 tar vxzf >/dev/null 2>&1
		if [ ! -d bak ]; then
			mkdir bak
		fi
		echo 'unpack complete!'
		echo 'move *.tar.gz to ./bak'
		mv *.tar.gz bak/
	fi
	
	echo
	echo "=====  unzip finished!  ====="
}

function deploy_all()
{
	echo "=====  start deploy!  ====="
	echo
	
	delploy_block CPM_V300R007 cpm
	delploy_block RTI_V300R007 rti
	
	deploy_db
	deploy_redis
	
	delploy_block cdnadapter_ cdnadapter
	delploy_block CI_V300R007 ci
	delploy_block CLS_V300R007 cls
	delploy_block CL_V300R007 cl
	delploy_block CG_V300R007 cg	
	delploy_block RTCL_V300R007 rtcl
	
	
	echo
	echo "=====  deploy finished!  ====="
}

function delploy_block()
{
	dir_name=$1
	block=$2
	echo "deploy ${block}"
	cd ${current_dir}
	if [ -d "${dir_name}"* ]; then
		mv "${dir_name}"* ${block}
		pullout ${block}
	fi
}

function deploy_db()
{
	cd ${current_dir}
	sed -i "s#cpmdb#${db_name}#g" ${current_dir}/cpm/db/createDB.sql
	sed -i "s#cpmdb#${db_name}#g" ${current_dir}/cpm/db/procedure.sql
	sed -i "s#cpmdb#${db_name}#g" ${current_dir}/rti/db/rti_CreateTab.sql
	
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "show databases;"|grep -w ${db_name} >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		echo
		echo "数据库服务器已存在同名数据库！请重新配置部署数据库名或手动移除该数据库"
		echo "脚本即将退出"
		exit
	fi
	
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "
		source cpm/db/createDB.sql;
		source cpm/db/procedure.sql;
		source rti/db/rti_CreateTab.sql;
	"
}

function deploy_redis()
{
	mv ${current_dir}/redis_2.4.7 ${current_dir}/redis
	if [ ${redhat_ver}x = "5"x ]; then
		mv ${current_dir}/redis/rhel5.5_64/redis* ${current_dir}/redis/
	elif [ ${redhat_ver}x = "5"x ]; then
		mv ${current_dir}/redis/rhel6.2_64/redis* ${current_dir}/redis/
	else
		echo "检测redhat版本不在支持范围内"
		echo "脚本即将退出"
		exit
	fi
	
	if [ ! -d ${current_dir}/bak ]; then
		mkdir -p ${current_dir}/bak
	fi
	mv ${current_dir}/redis/rhel* ${current_dir}/bak/
}

function pullout()
{
	block=$1
	
	if [ "${block}" == "ci" -o "${block}" == "rti" -o "${block}" == "rtcl" ]; then
		block2=`tr '[A-Z]|[a-z]' '[a-z]|[A-Z]' <<<"${block}"`		
	else
		block2="${block}"
	fi
	
	#if [ -d ${current_dir}/${block} ]; then
	#	cd ${block}/version/${block2}/
	#fi
	
	if [ -d ${current_dir}/${block}/version/${block2}/bin -o -f ${current_dir}/${block}/version/${block2}/Transceiver ]; then
		mv ${current_dir}/${block}/version/${block2}/* ${current_dir}/${block}/
	fi	
			
	if [ -d ${current_dir}/${block}/version ]; then	
		remove_force ${current_dir}/${block}/version
	fi
}

function remove_force()
{
	echo "remove directory " $1
	rm -rf $1
}









function configure_all()
{
	echo "=====  start configure!  ====="
	echo

	configure_redis
	#configure_cdnadapter
	configure_ci
	configure_cpm
	configure_cls
	configure_cl
	#configure_cg
	configure_rti
	configure_rtcl
	
	echo
	echo "=====  configure finished!  ====="
}

function configure_redis
{
	echo "configure redis"
	block=redis
	sed -i "s#6379#${redis_port}#g" redis/redis.conf
	sed -i "s#daemonize no#daemonize yes#g" redis/redis.conf
	sed -i "s/save\ 900/#save\ 900/g" redis/redis.conf
	sed -i "s/save\ 300/#save\ 300/g" redis/redis.conf
	sed -i "s/save\ 60/#save\ 60/g" redis/redis.conf
	mv redis/redis.conf redis/redis"${redis_port}".conf
}

function configure_ci()
{
	echo "configure ci"
	block=ci
	sed -i "s#${ci_old_ip}#${ci_ip}#g" ci/config/config.xml
	sed -i "s#${cpm_old_ip}:${cpm_old_port}#${cpm_ip}:${cpm_port}#g" ci/config/config.xml
	sed -i "s#${ci_old_port}#${ci_port}#g" ci/config/config.xml
	
	sed -i "s#/mpeg/#${home_dir}/${block}/#g" ci/config/config.xml
	sed -i "s#/</FtpURL#/${block}/</FtpURL#g" ci/config/config.xml
	sed -i "s#>${block}<#>${ftp_user_pswd}<#g" ci/config/config.xml	
	mkdir -p "${home_dir}/${block}"
	echo "*ftp setting need configure manually!"
	echo "请手动安装vsftp，并创建用户${ftp_user_pswd}，将用户主目录设为${home_dir}，密码同用户名"
	echo "创建用户参考：useradd -G root -d ${home_dir} -M ${ftp_user_pswd}"
	echo "修改用户密码参考：passwd ${ftp_user_pswd}"
	echo "按任意键继续..."
	get_char
}

function configure_cpm()
{
	echo "configure cpm"	
	sed -i "s#${cpm_old_port}#${cpm_port}#g" cpm/config/config.xml
	#??
	sed -i "s#${cpm_old_ip}#${cpm_ip}#g" cpm/config/config.xml
	sed -i "s#${db_old_ip}#${db_ip}#g" cpm/config/config.xml
	sed -i "s#>${db_old_name}<#>${db_name}<#g" cpm/config/config.xml
}

function configure_cls()
{
	echo "configure cls"	
	sed -i "s#${redis_old_addr}#${redis_ip}:${redis_port}#g" cls/config/config.xml
	sed -i "s#${cls_old_port}#${cls_port}#g" cls/config/config.xml
}

function configure_cl()
{
	echo "configure cl"
	block=cl
	#SVN配置文件本身有误，适应性纠正
	cpm_old_ip_bak=${cpm_old_ip}
	cpm_old_ip=${cl_old_ip}
	sed -i "s#${cl_old_addr}#${cl_ip}:${cl_port}#g" cl/config/config.xml
	sed -i "s#${cl_old_saddr}#${cl_ip}:${cl_sport}#g" cl/config/config.xml
	sed -i "s#${cpm_old_ip}:${cpm_old_port}#${cpm_ip}:${cpm_port}#g" cl/config/config.xml
	sed -i "s#${cls_old_ip}:${cls_old_port}#${cls_ip}:${cls_port}#g" cl/config/config.xml
	
	#ftp
	sed -i "s#/mpeg/#${home_dir}/${block}/#g" cl/config/config.xml
	sed -i "s#/</expose_url#/${block}/</expose_url#g" cl/config/config.xml
	sed -i "s#cl_ftp:cl_ftp#${ftp_user_pswd}:${ftp_user_pswd}#g" cl/config/config.xml	
	
	sed -i "s#/diskb/mpeg1/#${home_dir}/${block}/#g" cl/config/config.xml
	sed -i "s#/diskb/mpeg2/#${home_dir}/rtcl/HNWS#g" cl/config/config.xml
	sed -i "s#/diskb/mpeg3/#${home_dir}/rtcl/GDWS#g" cl/config/config.xml
	
	sed -i "s#${cl_old_ip}#${cl_ip}#g" cl/config/config.xml	
	cpm_old_ip=${cpm_old_ip_bak}
}

function configure_rti()
{
	echo "configure rti"
	sed -i "s#${rti_old_ip}#${rti_ip}#g" rti/config/config.xml
	sed -i "s#${rti_old_port}#${rti_port}#g" rti/config/config.xml
		
	sed -i "s#239.9.9.9:10002#239.9.9.9:50000#g" rti/config/config.xml
	sed -i "s#${rtcl_old_ip}:${rtcl_old_port}#${rtcl_ip}:${rtcl_port}#g" rti/config/config.xml
	sed -i "s#/diskb/RTCL/CCTV#${home_dir}/rtcl/HNWS#g" rti/config/config.xml
	sed -i "s#ftp://rtcl:rtcl@172.20.15.10:21/#ftp://${ftp_user_pswd}:${ftp_user_pswd}@${rtcl_ip}:21/rtcl/HNWS#g" rti/config/config.xml
	
	sed -i "s#${db_old_ip}#${db_ip}#g" rti/config/config.xml
	sed -i "s#>${db_old_name}<#>${db_name}<#g" rti/config/config.xml
	echo "*RTCL channel need add more manually"
}

function configure_rtcl()
{
	echo "configure rtcl"
	sed -i "s#${rtcl_old_port}#${rtcl_port}#g" rtcl/Transceiver.xml
}

function get_char()
{
	SAVEDSTTY=`stty -g`
	stty -echo
	stty raw
	dd if=/dev/tty bs=1 count=1 2> /dev/null
	stty -raw
	stty echo
	stty $SAVEDSTTY
}











function start_block()
{
	block=$1
	echo "start ${block}"
	cd ${current_dir}/${block}
	if [ "${block}"x = "redis"x ]; then
		./redis-server redis"${redis_port}".conf
	else
		if [ -d bin ]; then
			cd bin
		fi
		chmod 755 *.sh
		sh start.sh;
	fi
	echo
}

function stop_block()
{
	block=$1
	echo "stop ${block}"
	cd ${current_dir}/${block}
	if [ "${block}"x = "redis"x ]; then
		killall -9 redis-server
	else
		if [ -d bin ]; then
			cd bin
		fi
		chmod 755 *.sh
		sh stop.sh;
	fi	
}

function start_all()
{
	echo "=====  start run!  ====="
	echo
	
	start_block cpm
	start_block redis
	start_block cls
	start_block cl
	start_block ci
	start_block rtcl
	start_block rti
	#start_block cdnadapter
	
	echo
	echo "=====  run finished!  ====="
}






function pass()
{
	module=$1
	result="启动成功"
    echo -e "\033[32;1m *${module}${result}\033[0m"
}

function fail()
{
    module=$1
	result="启动可能失败,请手动检查"
    echo -e "\033[31;1m *${module}${result}\033[0m"
}
#检查Redis运行状态
function cdn_redis_running()
{
	redis_running=`ps -ef|grep "redis-server ./redis${redis_port}.conf"|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|wc -l`
	refer=$1
	if [ "$redis_running"x = "$refer"x ];then
		pass "redis"
	else
		fail "redis"
	fi
}
function cdn_cpm_running()
{
	cpm_running=`ps -ef| grep -w 'cpm'| grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'| wc -l`
	refer=$1
	if [ "$cpm_running"x = "$refer"x ];then
		pass "cpm"
	else
		fail "cpm"
	fi
}
function cdn_cls_running()
{
	cls_running=`ps -ef|grep -w 'cls'|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|grep -v 'clserver'|wc -l`
	refer=$1
	if [ "$cls_running"x = "$refer"x ];then
		pass "cls"
	else
		fail "cls"
	fi
}
function cdn_cl_running()
{
    cl_running=`ps -ef|grep 'clserver'|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|wc -l`
	refer=$1
	if [ "$cl_running"x = "$refer"x ];then
		pass "cl"
	else
		fail "cl"
	fi
}
function cdn_ci_running()
{
	ci_running=`ps -ef|grep -w 'CI'|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|wc -l`
	refer=$1
	if [ "$ci_running"x = "$refer"x ];then
		pass "ci"
	else
		fail "ci"
	fi
}
function cdn_rtcl_running()
{
	rtcl_running=`ps -ef|grep 'Transceiver'|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|wc -l`
	refer=$1
	if [ "$rtcl_running"x = "$refer"x ];then
		pass "rtcl"
	else
		fail "rtcl"
	fi
}
function cdn_rti_running()
{
    rti_running=`ps -ef|grep -w 'rti'|grep -v 'grep'|grep -v 'bash'|grep -v 'defunct'|wc -l`
	refer=$1
	if [ "$rti_running"x = "$refer"x ];then
		pass "rti"
	else
		fail "rti"
	fi
}
function check_all()
{
	cdn_redis_running 1
	cdn_cpm_running 2
	cdn_cls_running 2
	cdn_cl_running 2
	cdn_rtcl_running 2
	cdn_rti_running 2
	cdn_ci_running 2
	
	echo
	echo "=====  check finished!  ====="
}




check_para

echo
echo "===============  CDN.R007 基本模块自动部署  ==============="
echo "若用做性能测试，请手动修改cl配置文件"
echo "对于CG、CDNAdapter等模块请手动安装"
echo "请确定已将 redis/ci/cl/cls/cpm/rtcl/rti 包放入此目录，按任意键继续..."


get_char
unzip;
deploy_all;
configure_all;
start_all;
check_all
