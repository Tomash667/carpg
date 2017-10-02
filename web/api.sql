CREATE TABLE ca_stats
(
	id int IDENTITY(1,1) PRIMARY KEY,
	ouid varchar(32) NOT NULL,
	uid varchar(32) NOT NULL,
	version int NOT NULL,
	winver int NOT NULL,
	ram float NOT NULL,
	cpu_flags int NOT NULL,
	vs_ver int NOT NULL,
	ps_ver int NOT NULL,
	insertdate datetime NOT NULL,
	ip varchar(50) NOT NULL
)

CREATE VIEW ca_stats AS
select 
uid,
CONCAT(CAST((version&0xFF0000)>>16 AS CHAR(3)),'.',CAST((version&0xFF00)>>8 AS CHAR(3)),'.',CAST((version&0xFF) AS CHAR(3))) as 'version',
CONCAT(CAST((winver&0xFFFF0000)>>16 AS CHAR(3)),'.',CAST((winver&0xFFFF) AS CHAR(3))) as 'winver',
ram,
cpu_flags,
CONCAT(CAST((vs_ver&0xFF00)>>8 AS CHAR(3)),'.',CAST((vs_ver&0xFF) AS CHAR(3))) as 'vs_ver',
CONCAT(CAST((ps_ver&0xFF00)>>8 AS CHAR(3)),'.',CAST((ps_ver&0xFF) AS CHAR(3))) as 'vs_ver',
insertdate,
ip,
(cpu_flags&0x1)!=0 as x64,
(cpu_flags&0x4)!=0 as sse,
(cpu_flags&0x8)!=0 as sse2
from ca_stats
