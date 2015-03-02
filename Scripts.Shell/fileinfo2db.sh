#!/bin/bash

db_name=$1
separator="_"
db_ip=172.21.11.169
db_user=root
db_pswd=123456

# create database and table if not exist
function create_db()
{
	if [ "${db_name}"x = x ]; then
		echo
		echo "Usage: fileinfo2db.sh <db_name>"
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
			node_id INT(11) NULL DEFAULT NULL,
			dir_id INT(11) NULL DEFAULT NULL,
			flag INT(11) NULL DEFAULT NULL,			
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
	cat ${file_item}|grep '-'|awk -F " " -v anode="$node" -v adir="$dir" '
	{
		printf ("insert into phfile_info (node, dir, filename, filesize) values('\''%s'\'', '\''%s'\'', '\''%s'\'', '\''%s'\'');\r\n", anode, adir, $NF, $5);
	}
	'>>${file_item}.sql
	#chmod 755 ${file_item}.sql
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
	mysql -h${db_ip} -u${db_user} -p${db_pswd} -D${db_name} -N -e "
		update phfile_info set node_id=substr(node,3) where node like 'cl%';
		update phfile_info set node_id=substr(node,3) where node like 'cg%';
		update phfile_info set dir_id=substr(dir,5)	where dir like 'mpeg%';
	"
}

create_db
insert2db
update_db
