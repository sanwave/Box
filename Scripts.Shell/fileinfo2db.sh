#!/bin/bash

# 
# 说明
# 注意：
# 0. 请使用 ls --full-time /mpeg/mpeg1 获取CDN媒资文件列表并重定向到类似cl1_mpeg1.txt命名的文本文件中，以及CDN业务数据库作为输入
# 1. 手动导入数据库并执行此脚本
# 2. CDN.R005 需将node_savedir表中所有存储非实时目录配置ftp地址；
#    CDN.R007 需在脚本执行过程中手动设置CDN节点实际存储目录
# 3. CDN.R005所有的媒资附加文件无法校验完整性，全部覆盖重新生成（可去掉--force参数只作补充）；
#    CDN.R007损坏的媒资附加文件覆盖重新生成（不包括主媒资文件损坏的媒资附加文件，R005同）
# 4. 若输入媒资列表文件不完全，则可能出现大量 file_broken_need_clean 状态的文件，因无法确认完整拷贝源，故无法恢复
# 


db_ip=$1
db_name=$2
separator="_"
db_user=root
db_pswd=123456
handle_dir=`pwd`/solution

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

# create database and table if not exist
function prepare_env()
{
	if [ "${db_ip}"x = x -o "${db_name}"x = x ]; then
		echo "Usage: fileinfo2db.sh <db_ip> <db_name>"
		exit
	fi
	
	echo "此脚本可能需要执行较长时间，请耐心等待几分钟..."
	echo "                                                    来杯香草拿铁！"

	
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "
		CREATE DATABASE IF NOT EXISTS ${db_name};
		USE ${db_name};
		DROP TABLE IF EXISTS phfile_info;
		CREATE TABLE phfile_info (
			node VARCHAR(50) NULL DEFAULT NULL,
			dir VARCHAR(50) NULL DEFAULT NULL,
			filename VARCHAR(250) NULL DEFAULT NULL,
			filesize BIGINT NULL DEFAULT NULL,
			flag VARCHAR(23) NULL DEFAULT NULL,
			initial_size BIGINT NULL DEFAULT NULL,
			node_id INT(11) NULL DEFAULT NULL,
			dir_id INT(11) NULL DEFAULT NULL,			
			INDEX emp_node (node),
			INDEX emp_filename (filename),
			INDEX emp_node_id (node_id)
		)
		COLLATE='utf8_general_ci'
		ENGINE=InnoDB
		;
	"
}

function create_sql_cmd()
{
	file_item=$1
	node=`echo ${file_item}|awk -F ${separator} '{print $1}'`
	dir=`echo ${file_item}|awk -F ${separator} '{print $2}'|awk -F '.' '{print $1}'`
	
	echo "use ${db_name};">${file_item}.sql	
	cat ${file_item}|grep '^-'|awk -F " " -v anode="$node" -v adir="$dir" '
	{
		printf ("insert into phfile_info (node, dir, filename, filesize) values('\''%s'\'', '\''%s'\'', '\''%s'\'', '\''%s'\'');\r\n", anode, adir, $NF, $5);
	}
	'>>${file_item}.sql
}

function execute_sql_cmd()
{
	file_item=$1
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "show databases;"|grep ${db_name} >/dev/null 2>&1
	if [ $? -eq 0 ]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			source ${file_item};
		"
	fi
}

# 将文件列表信息导入到数据库，待分析处理
function import_data()
{
	# <node_name>_<dir_name>.txt, e.g. cl158_mpeg1.txt
	for item in $(ls *.txt | xargs -n 1)
	do
		create_sql_cmd $item
	done

	for item in $(ls *.txt.sql | xargs -n 1)
	do
		execute_sql_cmd $item
		rm -f $item
	done
}

# 
# phfile_info flag explanation
# 
# file_is_ok                 文件信息与数据库记录比对一致
# source_file_broken         数据库记录损坏，无法验证文件完整性。此文件所属媒资需要清理
# no_record_need_clean       数据库不存在该文件信息。此文件若为主媒资文件，则需清理
# file_broken                文件大小与数据库记录大小不匹配，该文件可经CDN其他节点恢复
# file_broken_need_clean     文件大小与数据库记录大小不匹配，因CDN不存在此文件完整拷贝，此文件无法恢复，需清理
# attach_file_need_clean     主媒资文件不存在或无法/无需恢复的媒资附属文件，需清理
# unknown_attach_file        CDN.R005下正常或可恢复的主媒资的附属文件，数据库无附属文件记录，故标记此状态
# lost_need_recover          媒资文件丢失，按文件大小为0处理
# 

# 分析导入的数据
function analysis()
{
	# for cdn r005
	if [[ "${db_name}"x = *"coccdn"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			update phfile_info set node_id=substr(node,4) where node like 'src%';
			update phfile_info set node_id=substr(node,5) where node like 'edge%';
			update phfile_info set dir_id=substr(dir,5)	where dir like 'mpeg%';
			update phfile_info p inner join vod_program v on (p.filename=v.filename and p.node_id=v.node_id and p.dir_id=v.file_dir_id) set p.initial_size=v.file_size;
			update phfile_info p inner join vod_program v on (p.filename=v.filename) inner join vod_edge_program e on (v.content_id=e.content_id and p.node_id=e.node_id and p.dir_id=e.path_idx) set p.initial_size=v.file_size;
			
			insert into phfile_info (node, dir, filename, filesize, flag, initial_size, node_id, dir_id)
			select concat('src', v.node_id), concat('mpeg', v.file_dir_id), v.filename, '0', 
			    'lost_need_recover', v.file_size, v.node_id, v.file_dir_id 
			  from vod_program v 
			    left join phfile_info p on (v.filename=p.filename and v.node_id=p.node_id)
			  where v.realplay_flag=0 and v.file_state=4 and isnull(p.filename)
			union
			select concat('edge', e.node_id), concat('mpeg', e.path_idx), v.filename, '0', 
			    'lost_need_recover', v.file_size, e.node_id, e.path_idx 
			  from vod_edge_program e inner join vod_program v on (e.content_id=v.content_id)
			    left join phfile_info p on (v.filename=p.filename and e.node_id=p.node_id)
			  where v.realplay_flag=0 and e.content_state=4 and isnull(p.filename);
		"
	fi
	
	# for cdn r007
	if [[ "${db_name}"x = *"cpmdb"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			update phfile_info set node_id=substr(node,3) where node like 'cl%';
			update phfile_info set node_id=substr(node,3) where node like 'cg%';
			update phfile_info set dir_id=substr(dir,5)	where dir like 'mpeg%';
			update phfile_info p inner join file_info f on (p.filename=f.file_name) inner join file_dist d on(f.filename=d.filename and p.node=d.node_name) set p.initial_size=f.file_size;
			
			insert into phfile_info (`node`, `dir`, `filename`, `filesize`, `flag`, `initial_size`, `node_id`, `dir_id`)
            select d.node_name, 'mpeg0', d.file_name, '0', 'lost_need_recover', f.file_size, substr(d.node_name,3), 0
              from file_dist d
                inner join file_info f on (d.file_name=f.file_name)
                left join phfile_info p on (d.file_name=p.filename and d.node_name=p.node)
              where f.file_state=2 and f.file_name not in ('.', '..', 'temp') and isnull(p.filename);
		"
	fi
	
	# compare and analysis file info
	# find and remark the attach file whois main asset file can(or need) not recover
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "		
		update phfile_info p set p.flag='no_record_need_clean' where ISNULL(p.initial_size);
		update phfile_info p set p.flag='source_file_broken' where p.initial_size = 0;
		update phfile_info p set p.flag='file_is_ok' where p.filesize=p.initial_size and p.initial_size>0;
		update phfile_info p set p.flag='file_broken' where ISNULL(p.flag);
		update phfile_info p left join phfile_info ph on (p.filename=ph.filename and p.node!=ph.node and ph.flag='file_is_ok') set p.flag='file_broken_need_clean' where p.flag in('file_broken', 'lost_need_recover') and ISNULL(ph.flag);
		
		update phfile_info p left join phfile_info ph on (substring_index(p.filename, '.', 2)=ph.filename and p.node=ph.node) set p.flag='attach_file_need_clean' where (isnull(ph.flag) or ph.flag in ('no_record_need_clean', 'source_file_broken', 'file_broken_need_clean')) and (p.filename like '%.idx' or p.filename like '%.top');
		update phfile_info p left join phfile_info ph on (substring_index(p.filename, '_8_', 1)=ph.filename and p.node=ph.node) set p.flag='attach_file_need_clean' where (isnull(ph.flag) or ph.flag in ('no_record_need_clean', 'source_file_broken', 'file_broken_need_clean')) and p.filename like '%\_8\_%';
		update phfile_info p left join phfile_info ph on (substring_index(p.filename, '_16_', 1)=ph.filename and p.node=ph.node) set p.flag='attach_file_need_clean' where (isnull(ph.flag) or ph.flag in ('no_record_need_clean', 'source_file_broken', 'file_broken_need_clean')) and p.filename like '%\_16\_%';
		update phfile_info p left join phfile_info ph on (substring_index(p.filename, '_32_', 1)=ph.filename and p.node=ph.node) set p.flag='attach_file_need_clean' where (isnull(ph.flag) or ph.flag in ('no_record_need_clean', 'source_file_broken', 'file_broken_need_clean')) and p.filename like '%\_32\_%';				
	"
	
	# for cdn r005
	if [[ "${db_name}"x = *"coccdn"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			alter table vod_program add INDEX emp_filename (filename);
		" >/dev/null 2>&1
		
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			update node_savedir set ftp_url=concat(ftp_url,'/') where ftp_url not like '%/';
			update node_savedir set real_dir=concat(real_dir,'/') where real_dir not like '%/';
			
			update phfile_info p inner join vod_program v on (substring_index(p.filename, '.', 2)=v.filename)
		      set p.flag='unknown_attach_file' where p.flag='no_record_need_clean' and ((p.filename like '%.idx' or p.filename like '%.top'));
		    update phfile_info p inner join vod_program v on (substring_index(p.filename, '_8_', 1)=v.filename) 
		      set p.flag='unknown_attach_file' where p.flag='no_record_need_clean' and p.filename like '%\_8\_%';
		    update phfile_info p inner join vod_program v on (substring_index(p.filename, '_16_', 1)=v.filename) 
		      set p.flag='unknown_attach_file' where p.flag='no_record_need_clean' and p.filename like '%\_16\_%';
		    update phfile_info p inner join vod_program v on (substring_index(p.filename, '_32_', 1)=v.filename) 
		      set p.flag='unknown_attach_file' where p.flag='no_record_need_clean' and p.filename like '%\_32\_%';
		"
	fi
}

function make_excute_r005_db()
{
	# 清理主媒资文件源文件损坏及无法恢复的媒资，将该媒资置为待删除状态
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		select concat('update vod_program v set v.file_state=\'3\' where v.filename=\'', p.filename, '\' and v.node_id=\'', p.node_id, '\';',
		  'update vod_edge_program e inner join vod_program v on (e.content_id=v.content_id) set e.content_state=\'3\' where v.filename=\'', p.filename, '\' and e.node_id=\'', p.node_id, '\';')
		  from phfile_info p
		  where p.flag in ('source_file_broken', 'file_broken_need_clean') 
		into outfile '${handle_dir}/execute_db.sql';
	"
	sed "1i use coccdn;" -i ${handle_dir}/execute_db.sql
}

function make_excete_r005_shell()
{
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		select node_id from phfile_info group by node_id into outfile '${handle_dir}/variable.txt';
	"
	
	# 清理不能恢复或无需恢复的媒资文件
	# 其中，source_file_broken，file_broken_need_clean状态的主媒资文件清理与上述数据库状态标记重复，
	# attach_file_need_clean状态的附加文件清理会与上述数据库标记source_file_broken状态的媒资清理重复 
	for node_id in $(cat ${handle_dir}/variable.txt)
	do
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			select concat('rm -f ', n.real_dir, p.filename)
              from phfile_info p inner join node_savedir n on (p.node_id=n.node_id and p.dir_id=n.path_idx)
              where p.flag in ('source_file_broken', 'no_record_need_clean', 'file_broken_need_clean', 'attach_file_need_clean') and p.node_id in (${node_id}) 
			into outfile '${handle_dir}/clean_${node_id}.sh';
		"
		sed "1i #!/bin/bash" -i ${handle_dir}/clean_${node_id}.sh
	done
	
	# 恢复媒资文件
	for node_id in $(cat ${handle_dir}/variable.txt)
	do
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			select concat('wget -c ', ns.ftp_url, ph.filename, ' -O ', nc.real_dir, p.filename)
		      from phfile_info p left join phfile_info ph on (p.filename=ph.filename and p.node!=ph.node and ph.flag='file_is_ok') 
		      inner join node_savedir ns on (ph.node_id=ns.node_id and ph.dir_id=ns.path_idx)
		      inner join node_savedir nc on (p.node_id=nc.node_id and p.dir_id=nc.path_idx)
		      where p.flag in ('file_broken', 'lost_need_recover') and !ISNULL(ph.flag) and p.node_id in (${node_id})
			into outfile '${handle_dir}/recover_${node_id}.sh';
		"
		sed "1i #!/bin/bash" -i ${handle_dir}/recover_${node_id}.sh
	done
	
	# 构造重新生成媒资附加文件的脚本，由于现有的附加文件无法验证，故只能覆盖
	for node_id in $(cat ${handle_dir}/variable.txt)
	do
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "			
			select concat('./IndexCreator -p ', v.provider_id, ' -a ', v.asset_id, ' -s 8 --usets --force -f ', n.real_dir, v.filename)
			  from vod_program v 
			  inner join node_savedir n on (v.node_id=n.node_id and v.file_dir_id=n.path_idx)
			  where v.node_id in (${node_id}) and v.file_state='4'
			union
			select concat('./IndexCreator -p ', v.provider_id, ' -a ', v.asset_id, ' -s 8 --usets --force -f ', n.real_dir, v.filename)
			  from vod_edge_program e
			  inner join vod_program v on (e.content_id=v.content_id)
			  inner join node_savedir n on (e.node_id=n.node_id and e.path_idx=n.path_idx)
			  where e.node_id in (${node_id}) and v.file_state='4'
			into outfile '${handle_dir}/createfile_${node_id}.sh';
		"
		sed "1i #!/bin/bash" -i ${handle_dir}/createfile_${node_id}.sh
	done
}

function make_excute_r007_db()
{
	# 清理源文件损坏及无法恢复的媒资文件，将该文件置为待删除状态
	# 未校验媒资的完整性，即主媒资文件与附加文件是否齐全
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		select concat('update file_dist d set d.delete_state=\'1\' where d.filename=\'', p.filename, '\' and d.node_name=\'', p.node, '\';')
		  from phfile_info p
		  where flag in ('source_file_broken', 'file_broken_need_clean', 'attach_file_need_clean') 
		into outfile '${handle_dir}/execute_db.sql';
	"
	sed "1i use cpmdb;" -i ${handle_dir}/execute_db.sql
}

function make_excete_r007_shell()
{
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		select node_id from phfile_info group by node_id into outfile '${handle_dir}/variable.txt';
	"
	# 构造并填充CDN.R007的存储节点目录表
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		DROP TABLE IF EXISTS node_path;
		CREATE TABLE `node_path` (
		  `node` VARCHAR(50) NULL DEFAULT NULL,
		  `dir` VARCHAR(50) NULL DEFAULT NULL,
		  `path` VARCHAR(100) NULL DEFAULT NULL
		)
		COLLATE='utf8_general_ci'
		ENGINE=MyISAM
		;
		insert into `node_path`(`node`, `dir`)
		  select p.node, p.dir
		  from phfile_info p
		  group by p.node, p.dir
		  order by p.node, p.dir;
	"
	
	echo "请手动配置CDN节点实际存储目录，数据库：${db_name}，表：node_path，列：path"
	echo "请手动指定lost_need_recover状态的文件目录"
	get_char
	get_char
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		update node_path set path=concat(path,'/') where path not like '%/';
	"
	######################### 此处会暂停，需手动配置CDN节点实际存储目录 #########################
	
	# 清理数据库中无记录的媒资文件
	for node_id in $(cat ${handle_dir}/variable.txt)
	do
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			select concat('rm -f ', n.real_dir, p.filename)
              from phfile_info p inner join node_path n on (p.node=n.node and p.dir=n.dir)
              where p.flag in ('no_record_need_clean') and p.node_id in (${node_id})
			into outfile '${handle_dir}/clean_${node_id}.sh';
		"
		sed "1i #!/bin/bash" -i ${handle_dir}/clean_${node_id}.sh
	done
	
	# 恢复损坏的媒资文件
	# lost_need_recover状态的文件因无法确定恢复目录可能需要手动恢复
	for node_id in $(cat ${handle_dir}/variable.txt)
	do
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			select concat('wget -c ', ns.ftp_url, ph.filename, ' -O ', nc.real_dir, p.filename)
		      from phfile_info p left join phfile_info ph on (p.filename=ph.filename and p.node!=ph.node and ph.flag='file_is_ok') 
		      inner join node_path ns on (ph.node=ns.node and ph.dir=ns.dir)
		      inner join node_path nc on (p.node=nc.node and p.dir=nc.dir)
		      where p.flag in ('file_broken', 'lost_need_recover') and !ISNULL(ph.flag) and p.node_id in (${node_id})
			into outfile '${handle_dir}/recover_${node_id}.sh';
		"
		sed "1i #!/bin/bash" -i ${handle_dir}/recover_${node_id}.sh
	done
}

function handle()
{
	if [ -d ${handle_dir} ]; then
		rm -f ${handle_dir}/*
	else
		mkdir ${handle_dir}
	fi	
	
	chmod -R 777 ${handle_dir}
	if [[ "${db_name}"x = *"coccdn"*x ]]; then
		make_excute_r005_db
		make_excete_r005_shell
	else
		make_excute_r007_db
		make_excete_r007_shell
	fi
	chmod -R 755 ${handle_dir}
	
	rm -f ${handle_dir}/variable.txt
	find ${handle_dir} -size 0 -exec rm {} \;
}

prepare_env
import_data
analysis
handle

