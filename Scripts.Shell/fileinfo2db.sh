#!/bin/bash

db_ip=$1
db_name=$2
separator="_"
db_user=root
db_pswd=123456

# create database and table if not exist
function create_db()
{
	if [ "${db_ip}"x = x -o "${db_name}"x = x ]; then
		echo "Usage: fileinfo2db.sh <db_ip> <db_name>"
		exit
	fi
	
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -N -e "
		CREATE DATABASE IF NOT EXISTS ${db_name};
		USE ${db_name};
		DROP TABLE IF EXISTS phfile_info;
		CREATE TABLE phfile_info (
			node VARCHAR(50) NULL DEFAULT NULL,
			dir VARCHAR(50) NULL DEFAULT NULL,
			filename VARCHAR(250) NULL DEFAULT NULL,
			filesize BIGINT NULL DEFAULT NULL,
			flag VARCHAR(20) NULL DEFAULT NULL,
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

function insert2db()
{
	# <node_name>_<dir_name>.txt, e.g. cl158_mpeg1.txt
	for item in $(ls *.txt | xargs -n 1)
	do
		create_sql_cmd $item
	done

	for item in $(ls *.txt.sql | xargs -n 1)
	do
		execute_sql_cmd $item
	done
}

# set value of node_id and dir_id
function update_db()
{
	# for cdn r005
	if [[ "${db_name}"x = *"coccdn"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			update phfile_info set node_id=substr(node,4) where node like 'src%';
			update phfile_info set node_id=substr(node,5) where node like 'edge%';
			update phfile_info set dir_id=substr(dir,5)	where dir like 'mpeg%';
			update phfile_info p inner join vod_program v on (p.filename=v.filename) set p.initial_size=v.file_size;
		"
	fi
	
	# for cdn r007
	if [[ "${db_name}"x = *"cpmdb"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			update phfile_info set node_id=substr(node,3) where node like 'cl%';
			update phfile_info set node_id=substr(node,3) where node like 'cg%';
			update phfile_info set dir_id=substr(dir,5)	where dir like 'mpeg%';
			update phfile_info p inner join file_info f on (p.filename=f.file_name) set p.initial_size=f.file_size;
		"
	fi
	
	# compare and analysis file info
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "		
		update phfile_info p set p.flag='no_record_in_db' where ISNULL(p.initial_size);
		update phfile_info p set p.flag='source_file_is_null' where p.initial_size = 0;
		update phfile_info p set p.flag='file_is_ok' where p.filesize=p.initial_size and p.initial_size!=0;
		update phfile_info p left join phfile_info ph on (p.filename=ph.filename and ph.flag='file_is_ok') set p.flag='no_hope' where ISNULL(p.flag) and ISNULL(ph.flag);
		update phfile_info p set p.flag='file_broken' where ISNULL(p.flag);
	"
	
	# for cdn r005
	if [[ "${db_name}"x = *"coccdn"*x ]]; then
		mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
			alter table vod_program add INDEX emp_filename (filename);
			update phfile_info p set p.flag='file_broken' where p.flag='no_hope';
			update phfile_info p inner join vod_program v on (substring_index(p.filename, '.', 2)=v.filename)
			  set p.flag='unknown_attach_file' where flag='no_record_in_db' and ((p.filename like '%.idx' or p.filename like '%.top'));
			update phfile_info p inner join vod_program v on (substring_index(p.filename, '_8_', 1)=v.filename) 
			  set p.flag='unknown_attach_file' where flag='no_record_in_db';
			update phfile_info p inner join vod_program v on (substring_index(p.filename, '_16_', 1)=v.filename) 
			  set p.flag='unknown_attach_file' where flag='no_record_in_db';
			update phfile_info p inner join vod_program v on (substring_index(p.filename, '_32_', 1)=v.filename) 
			  set p.flag='unknown_attach_file' where flag='no_record_in_db';
		"
	fi
}


echo "此脚本可能需要执行较长时间，请耐心等待几分钟..."
echo "                                                    来杯香草拿铁！"

create_db
insert2db
update_db
