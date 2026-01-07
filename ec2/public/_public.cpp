/*
   freec++��ܳ����ļ��������˹��������ݽṹ���������ࡣ
   ����һ����ȫ��Դ�Ŀ�ܣ���ʹ����û���κ����ƣ���������ҵ��;�����ǣ���ʹ���ߵ���Ŀ�У��������Դ����
   ���ߣ���ũ�е�   ���ڣ�20190601
*/

#include "_public.h"  // �����Զ��庯��������ͷ�ļ�


int SNPRINTF(char *str,const size_t destlen, const char *fmt, ...)
{
  memset(str,0,destlen+1);

  va_list arg;

  va_start( arg, fmt );
  vsnprintf( str,destlen, fmt, arg );
  va_end( arg );
}

char *STRCPY(char* dest,const size_t destlen,const char* src)
{
  memset(dest,0,destlen+1); 

  if (strlen(src)>destlen) strncpy(dest,src,destlen);
  else strcpy(dest,src);

  return dest;
}

char *STRNCPY(char* dest,const size_t destlen,const char* src,size_t n)
{
  memset(dest,0,destlen+1); 

  if (n>destlen) strncpy(dest,src,destlen);
  else strncpy(dest,src,n);

  return dest;
}

char *STRCAT(char* dest,const size_t destlen,const char* src)
{
  memset(dest+strlen(dest),0,destlen-strlen(dest)+1); 
  
  int left=destlen-strlen(dest);

  int len=0;

  if (strlen(src)>left) len=left;
  else len=strlen(src);
  
  strncat(dest,src,len);

  return dest;
}

char *STRNCAT(char* dest,const size_t destlen,const char* src,size_t n)
{
  memset(dest+strlen(dest),0,destlen-strlen(dest)+1); 
  
  int left=destlen-strlen(dest);

  int len=0;

  if (n>left) len=left;
  else len=n;
  
  strncat(dest,src,len);

  return dest;
}

// ���ַ���·���л�ȡ�ļ���
std::string GetFileNameFromPathStr(const std::string& path)
{
    // �ҵ����һ����б�ܻ�б�ܵ�λ��
    std::size_t last_slash_idx = path.find_last_of("\\/");
    // ����ҵ��˷ָ��������طָ���֮����ַ���
    if (last_slash_idx != std::string::npos) {
        return path.substr(last_slash_idx + 1);
    } else {
        // ���û���ҵ��ָ�������������·��
        return path;
    }
}


// ��������ʱ��ת��Ϊ�ַ�����ʽ��ʱ�䣬��ʽ�磺"2019-02-08 12:05:08"�����ת���ɹ���������0��ʧ�ܷ���-1���������������£�
/*
int timetostr(const time_t ti,char *strtime)
{
  struct tm *sttm; 

  if ( (sttm=localtime(&ti))==0 ) return -1;

  sprintf(strtime,"%d-%02d-%02d %02d:%02d:%02d",\
          sttm->tm_year+1900,sttm->tm_mon+1,sttm->tm_mday,sttm->tm_hour,sttm->tm_min,sttm->tm_sec);
  
  return 0;
}
*/

// ���ַ�����ʽ��ʱ��ת��Ϊ������ʱ�䣬�������������£�
/*
int strtotime(const char *strtime,time_t *ti)
{
  char strtmp[11];

  //"2019-02-08 12:05:08"
  struct tm sttm; 

  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime,4);
  sttm.tm_year=atoi(strtmp)-1900;
  
  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime+5,2);
  sttm.tm_mon=atoi(strtmp)-1;
  
  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime+8,2);
  sttm.tm_mday=atoi(strtmp);
  
  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime+11,2);
  sttm.tm_hour=atoi(strtmp);
  
  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime+14,2);
  sttm.tm_min=atoi(strtmp);
  
  memset(strtmp,0,sizeof(strtmp));
  strncpy(strtmp,strtime+17,2);
  sttm.tm_sec=atoi(strtmp);

  *ti=mktime(&sttm);

  return *ti;
}
*/

CFile::CFile()   // ��Ĺ��캯��
{
  m_fp=0;
  m_bEnBuffer=true;
  memset(m_filename,0,sizeof(m_filename));
  memset(m_filenametmp,0,sizeof(m_filenametmp));
}

// �ر��ļ�ָ��
void CFile::Close() 
{
  if (m_fp==0) return;

  fclose(m_fp);  // �ر��ļ�ָ��

  m_fp=0;
  memset(m_filename,0,sizeof(m_filename));

  // ���������ʱ�ļ�����ɾ������
  if (strlen(m_filenametmp)!=0) remove(m_filenametmp);

  memset(m_filenametmp,0,sizeof(m_filenametmp));
}

// �ж��ļ��Ƿ��Ѵ�
bool CFile::IsOpened()
{
  if (m_fp==0) return false;

  return true;
}

// �ر��ļ�ָ�룬��ɾ���ļ�
bool CFile::CloseAndRemove()
{
  if (m_fp==0) return true;

  fclose(m_fp);  // �ر��ļ�ָ��

  m_fp=0;

  if (remove(m_filename) != 0) { memset(m_filename,0,sizeof(m_filename)); return false; }

  memset(m_filename,0,sizeof(m_filename));

  return true;
}

CFile::~CFile()   // �����������
{
  Close();
}

// ���ļ���������FOPEN��ͬ���򿪳ɹ�true��ʧ�ܷ���false
bool CFile::Open(const char *filename,const char *openmode,bool bEnBuffer)
{
  Close();

  if ( (m_fp=FOPEN(filename,openmode)) == 0 ) return false;

  memset(m_filename,0,sizeof(m_filename));

  strncpy(m_filename,filename,300);

  m_bEnBuffer=bEnBuffer;

  return true;
}

// רΪ���������ļ���������fopen��ͬ���򿪳ɹ�true��ʧ�ܷ���false
bool CFile::OpenForRename(const char *filename,const char *openmode,bool bEnBuffer)
{
  Close();

  memset(m_filename,0,sizeof(m_filename));
  strncpy(m_filename,filename,300);
  
  memset(m_filenametmp,0,sizeof(m_filenametmp));
  SNPRINTF(m_filenametmp,300,"%s.tmp",m_filename);

  if ( (m_fp=FOPEN(m_filenametmp,openmode)) == 0 ) return false;

  m_bEnBuffer=bEnBuffer;

  return true;
}

// �ر��ļ�������
bool CFile::CloseAndRename()
{
  if (m_fp==0) return false;

  fclose(m_fp);  // �ر��ļ�ָ��

  m_fp=0;

  if (rename(m_filenametmp,m_filename) != 0)
  {
    remove(m_filenametmp);
    memset(m_filename,0,sizeof(m_filename));
    memset(m_filenametmp,0,sizeof(m_filenametmp));
    return false;
  }

  memset(m_filename,0,sizeof(m_filename));
  memset(m_filenametmp,0,sizeof(m_filenametmp));

  return true;
}

// ����fprintf���ļ�д������
void CFile::Fprintf(const char *fmt, ... )
{
  if ( m_fp == 0 ) return;

  va_list arg;

  va_start( arg, fmt );
  vfprintf( m_fp, fmt, arg );
  va_end( arg );

  if ( m_bEnBuffer == false ) fflush(m_fp);
}

// ����fgets���ļ��ж�ȡһ�У�bDelCRT=trueɾ�����з���false��ɾ����ȱʡΪfalse
bool CFile::Fgets(char *strBuffer,const int ReadSize,bool bDelCRT)
{
  if ( m_fp == 0 ) return false;

  memset(strBuffer,0,ReadSize+1);

  if (fgets(strBuffer,ReadSize,m_fp) == 0) return false;

  if (bDelCRT==true)
  {
    DeleteRChar(strBuffer,'\n'); DeleteRChar(strBuffer,'\r');
  }

  return true;
}

// ���ļ��ļ��ж�ȡһ��
// strEndStr��һ�����ݵĽ�����־�����Ϊ�գ����Ի��з�"\n"Ϊ������־��
// ��Fgets��ͬ����������ɾ��������־
bool CFile::FFGETS(char *strBuffer,const int ReadSize,const char *strEndStr)
{
  return FGETS(m_fp,strBuffer,ReadSize,strEndStr);
}

// ����fread���ļ��ж�ȡ���ݡ�
size_t CFile::Fread(void *ptr, size_t size)
{
  if ( m_fp == 0 ) return -1;

  return fread(ptr,1,size,m_fp);
}

// ����fwrite���ļ���д����
size_t CFile::Fwrite(const void *ptr, size_t size )
{
  if ( m_fp == 0 ) return -1;

  size_t tt=fwrite(ptr,1,size,m_fp);

  if ( m_bEnBuffer == false ) fflush(m_fp);

  return tt;
}


// ���ļ��ļ��ж�ȡһ��
// strEndStr��һ�����ݵĽ�����־�����Ϊ�գ����Ի��з�"\n"Ϊ������־��
// ����������ɾ���еĽ�����־
bool FGETS(const FILE *fp,char *strBuffer,const int ReadSize,const char *strEndStr)
{
  char strLine[ReadSize+1];

  memset(strLine,0,sizeof(strLine));

  while (true)
  {
    memset(strLine,0,ReadSize+1);

    if (fgets(strLine,ReadSize,(FILE *)fp) == 0) break;

    // ��ֹstrBuffer���
    if ( (strlen(strBuffer)+strlen(strLine)) >= (unsigned int)ReadSize ) break;

    strcat(strBuffer,strLine);

    if (strEndStr == 0) return true;

    if (strstr(strLine,strEndStr)!= 0) return true;
  }

  return false;
}


CCmdStr::CCmdStr()
{
  m_vCmdStr.clear();
}

void CCmdStr::SplitToCmd(const string in_string,const char *in_sep,const bool bdeletespace)
{
  // ������еľ�����
  m_vCmdStr.clear();

  int iPOS=0;
  string srcstr,substr;

  srcstr=in_string;

  char str[2048];

  while ( (iPOS=srcstr.find(in_sep)) >= 0)
  {
    substr=srcstr.substr(0,iPOS);

    if (bdeletespace == true)
    {
      memset(str,0,sizeof(str));

      strncpy(str,substr.c_str(),2000);

      DeleteLRChar(str,' ');

      substr=str;
    }

    m_vCmdStr.push_back(substr);

    iPOS=iPOS+strlen(in_sep);

    srcstr=srcstr.substr(iPOS,srcstr.size()-iPOS);

  }

  substr=srcstr;

  if (bdeletespace == true)
  {
    memset(str,0,sizeof(str));

    strncpy(str,substr.c_str(),2000);

    DeleteLRChar(str,' ');

    substr=str;
  }

  m_vCmdStr.push_back(substr);

  return;
}

int CCmdStr::CmdCount()
{
  return m_vCmdStr.size();
}

bool CCmdStr::GetValue(const int inum,char *in_return)
{
  if (inum >= m_vCmdStr.size()) return false;

  strcpy(in_return,m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,char *in_return,const int in_len)
{
  memset(in_return,0,in_len+1);

  if (inum >= m_vCmdStr.size()) return false;

  if (m_vCmdStr[inum].length() > (unsigned int)in_len)
  {
    strncpy(in_return,m_vCmdStr[inum].c_str(),in_len);
  }
  else
  {
    strcpy(in_return,m_vCmdStr[inum].c_str());
  }

  return true;
}

bool CCmdStr::GetValue(const int inum,int *in_return)
{
  (*in_return) = 0;

  if (inum >= m_vCmdStr.size()) return false;

  (*in_return) = atoi(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,long *in_return)
{
  (*in_return) = 0;

  if (inum >= m_vCmdStr.size()) return false;

  (*in_return) = atol(m_vCmdStr[inum].c_str());

  return true;
}

bool CCmdStr::GetValue(const int inum,double *in_return)
{
  (*in_return) = 0;

  if (inum >= m_vCmdStr.size()) return false;

  (*in_return) = (double)atof(m_vCmdStr[inum].c_str());

  return true;
}

CCmdStr::~CCmdStr()
{
  m_vCmdStr.clear();
}

/* ɾ���ַ������ָ�����ַ� */
void DeleteLChar(char *in_string,const char in_char)
{
  if (in_string == 0) return;

  if (strlen(in_string) == 0) return;

  char strTemp[strlen(in_string)+1];

  int iTemp=0;

  memset(strTemp,0,sizeof(strTemp));
  strcpy(strTemp,in_string);

  while ( strTemp[iTemp] == in_char )  iTemp++;

  memset(in_string,0,strlen(in_string)+1);

  strcpy(in_string,strTemp+iTemp);

  return;
}

/* ɾ���ַ����ұ�ָ�����ַ� */
void DeleteRChar(char *in_string,const char in_char)
{
  if (in_string == 0) return;

  int istrlen = strlen(in_string);

  while (istrlen>0)
  {
    if (in_string[istrlen-1] != in_char) break;

    in_string[istrlen-1]=0;

    istrlen--;
  }
}

/* ɾ���ַ�������ָ�����ַ� */
void DeleteLRChar(char *in_string,const char in_char)
{
  DeleteLChar(in_string,in_char);
  DeleteRChar(in_string,in_char);
}


/*
   ȡ����ϵͳ��ʱ��
   out_stime��������
   in_interval��ƫ�Ƴ�������λ����
   ���صĸ�ʽ��fmt������fmtĿǰ��ȡֵ���£������Ҫ���������ӣ�
   yyyy-mm-dd hh24:mi:ss���˸�ʽ��ȱʡ��ʽ
   yyyymmddhh24miss
   yyyy-mm-dd
   yyyymmdd
   hh24:mi:ss
   hh24miss
   hh24:mi
   hh24mi
   hh24
   mi
*/
void LocalTime(char *out_stime,const char *in_fmt,const int in_interval)
{
  time_t  timer;
  struct tm nowtimer;

  time( &timer ); timer=timer+in_interval;
  nowtimer = *localtime ( &timer ); nowtimer.tm_mon++;

  if (in_fmt==0)
  {
    snprintf(out_stime,20,"%04u-%02u-%02u %02u:%02u:%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec);
    return;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd hh24:mi:ss") == 0)
  {
    snprintf(out_stime,20,"%04u-%02u-%02u %02u:%02u:%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec);
    return;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd hh24:mi") == 0)
  {
    snprintf(out_stime,17,"%04u-%02u-%02u %02u:%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min);
    return;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd hh24") == 0)
  {
    snprintf(out_stime,14,"%04u-%02u-%02u %02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour);
    return;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd") == 0)
  {
    snprintf(out_stime,11,"%04u-%02u-%02u",nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday); return;
  }

  if (strcmp(in_fmt,"yyyy-mm") == 0)
  {
    snprintf(out_stime,8,"%04u-%02u",nowtimer.tm_year+1900,nowtimer.tm_mon); return;
  }

  if (strcmp(in_fmt,"yyyymmddhh24miss") == 0)
  {
    snprintf(out_stime,15,"%04u%02u%02u%02u%02u%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec);
    return;
  }

  if (strcmp(in_fmt,"yyyymmddhh24mi") == 0)
  {
    snprintf(out_stime,13,"%04u%02u%02u%02u%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min);
    return;
  }

  if (strcmp(in_fmt,"yyyymmddhh24") == 0)
  {
    snprintf(out_stime,11,"%04u%02u%02u%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour);
    return;
  }

  if (strcmp(in_fmt,"yyyymmdd") == 0)
  {
    snprintf(out_stime,9,"%04u%02u%02u",nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday); return;
  }

  if (strcmp(in_fmt,"hh24miss") == 0)
  {
    snprintf(out_stime,7,"%02u%02u%02u",nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec); return;
  }

  if (strcmp(in_fmt,"hh24mi") == 0)
  {
    snprintf(out_stime,5,"%02u%02u",nowtimer.tm_hour,nowtimer.tm_min); return;
  }

  if (strcmp(in_fmt,"hh24") == 0)
  {
    snprintf(out_stime,3,"%02u",nowtimer.tm_hour); return;
  }

  if (strcmp(in_fmt,"mi") == 0)
  {
    snprintf(out_stime,3,"%02u",nowtimer.tm_min); return;
  }
}


CLogFile::CLogFile()
{
  m_tracefp = 0;
  memset(m_filename,0,sizeof(m_filename));
  memset(m_openmode,0,sizeof(m_openmode));
  m_bBackup=true;
  m_bEnBuffer=false;
}

CLogFile::~CLogFile()
{
  Close();
}

void CLogFile::Close()
{
  if (m_tracefp != 0)
  {
    fclose(m_tracefp); m_tracefp=0;
  }
}

// filename��־�ļ���
// openmode���ļ��ķ�ʽ��������־�ļ���Ȩ��,ͬ���ļ�����(FOPEN)ʹ�÷���һ��
// bBackup��true-���ݣ�false-�����ݣ��ڶ���̵ķ�������У����������й���һ����־�ļ���bBackup����Ϊfalse
// bEnBuffer:true-���û�������false-�����û�������������û���������ôд����־�ļ��е����ݲ�������д���ļ���
bool CLogFile::Open(const char *in_filename,const char *in_openmode,bool bBackup,bool bEnBuffer)
{
  if (m_tracefp != 0) { fclose(m_tracefp); m_tracefp=0; }

  m_bEnBuffer=bEnBuffer;

  memset(m_filename,0,sizeof(m_filename));
  strcpy(m_filename,in_filename);

  memset(m_openmode,0,sizeof(m_openmode));
  strcpy(m_openmode,in_openmode);

  if ((m_tracefp=FOPEN(m_filename,m_openmode)) == NULL) return false;

  m_bBackup=bBackup;

  return true;
}

// �����־�ļ�����MAXLOGFSIZE���ͱ�����
bool CLogFile::BackupLogFile()
{
  // ������
  if (m_bBackup == false) return true;

  if (m_tracefp == 0) return true;

  fseek(m_tracefp,0L,2);

  if (ftell(m_tracefp) > MAXLOGFSIZE*1024*1024)
  {
    fclose(m_tracefp); m_tracefp=0;

    char strLocalTime[21];
    memset(strLocalTime,0,sizeof(strLocalTime));
    LocalTime(strLocalTime,"yyyymmddhh24miss");

    char bak_filename[301];
    memset(bak_filename,0,sizeof(bak_filename));
    snprintf(bak_filename,300,"%s.%s",m_filename,strLocalTime);
    rename(m_filename,bak_filename);
printf("rename %s m_filename ok\n",bak_filename);
    if ((m_tracefp=FOPEN(m_filename,m_openmode)) == NULL) return false;
  }

  return true;
}

bool CLogFile::Write(const char *fmt,...)
{
  if (BackupLogFile() == false) return false;

  char strtime[20]; LocalTime(strtime);

  va_list ap;

  va_start(ap,fmt);

  if (m_tracefp == 0)
  {
    fprintf(stdout,"%s ",strtime);
    vfprintf(stdout,fmt,ap);
    if (m_bEnBuffer==false) fflush(stdout);
  }
  else
  {
    fprintf(m_tracefp,"%s ",strtime);
    vfprintf(m_tracefp,fmt,ap);
    if (m_bEnBuffer==false) fflush(m_tracefp);
  }

  va_end(ap);

  return true;
}

bool CLogFile::WriteEx(const char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);

  if (m_tracefp == 0)
  {
    vfprintf(stdout,fmt,ap);
    if (m_bEnBuffer==false) fflush(stdout);
  }
  else
  {
    vfprintf(m_tracefp,fmt,ap);
    if (m_bEnBuffer==false) fflush(m_tracefp);
  }

  va_end(ap);

  return true;
}

// �ر�ȫ�����źź��������
void CloseIOAndSignal()
{
  int ii=0;

  for (ii=0;ii<100;ii++)
  {
    signal(ii,SIG_IGN); close(ii);
  }
}

// ��ĳ�ļ���Ŀ¼��ȫ·���е�Ŀ¼����Ŀ¼���Լ���Ŀ¼�µĸ�����Ŀ¼
bool MKDIR(const char *filename,bool bisfilename)
{
  // ���Ŀ¼�Ƿ���ڣ���������ڣ��𼶴�����Ŀ¼
  char strPathName[301];

  for (int ii=1; ii<strlen(filename);ii++)
  {
    if (filename[ii] != '/') continue;

    memset(strPathName,0,sizeof(strPathName));
    strncpy(strPathName,filename,ii);

    if (access(strPathName,F_OK) == 0) continue;

    if (mkdir(strPathName,00755) != 0) return false;
  }

  if (bisfilename==false)
  {
    if (access(filename,F_OK) != 0)
    {
      if (mkdir(filename,00755) != 0) return false;
    }
  }

  return true;
}

// ����fopen�������ļ�������ļ����а�����Ŀ¼�����ڣ��ʹ���Ŀ¼
FILE *FOPEN(const char *filename,const char *mode)
{
  if (MKDIR(filename) == false) return NULL;

  return fopen(filename,mode);
}

// ��ȡ�ļ��Ĵ�С�������ֽ���
int FileSize(const char *in_FullFileName)
{
  struct stat st_filestat;

  if (stat(in_FullFileName,&st_filestat) < 0) return -1;

  return st_filestat.st_size;
}

// �����ļ����޸�ʱ������
int UTime(const char *filename,const char *mtime)
{
  struct utimbuf stutimbuf;

  stutimbuf.actime=stutimbuf.modtime=strtotime(mtime);

  return utime(filename,&stutimbuf);
}

// ���ַ�����ʽ��ʱ��ת��Ϊtime_t
// stimeΪ�����ʱ�䣬��ʽ���ޣ���һ��Ҫ����yyyymmddhh24miss
time_t strtotime(const char *stime)
{
  char strtime[21],yyyy[5],mm[3],dd[3],hh[3],mi[3],ss[3];
  memset(strtime,0,sizeof(strtime));
  memset(yyyy,0,sizeof(yyyy));
  memset(mm,0,sizeof(mm));
  memset(dd,0,sizeof(dd));
  memset(hh,0,sizeof(hh));
  memset(mi,0,sizeof(mi));
  memset(ss,0,sizeof(ss));

  PickNumber(stime,strtime,false,false);

  if (strlen(strtime) != 14) return -1;

  strncpy(yyyy,strtime,4);
  strncpy(mm,strtime+4,2);
  strncpy(dd,strtime+6,2);
  strncpy(hh,strtime+8,2);
  strncpy(mi,strtime+10,2);
  strncpy(ss,strtime+12,2);

  struct tm time_str;

  time_str.tm_year = atoi(yyyy) - 1900;
  time_str.tm_mon = atoi(mm) - 1;
  time_str.tm_mday = atoi(dd);
  time_str.tm_hour = atoi(hh);
  time_str.tm_min = atoi(mi);
  time_str.tm_sec = atoi(ss);
  time_str.tm_isdst = 0;

  return mktime(&time_str);
}

// ��һ���ַ�������ȡ���֣�bWithSign==true��ʾ�������ţ�bWithDOT==true��ʾ����Բ��
void PickNumber(const char *strSrc,char *strDst,const bool bWithSign,const bool bWithDOT)
{
  char strtemp[1024];
  memset(strtemp,0,sizeof(strtemp));
  strncpy(strtemp,strSrc,1000);
  DeleteLRChar(strtemp,' ');

  // Ϊ�˷�ֹstrSrc��strDstΪͬһ���������������strDst���ܳ�ʼ��

  // �ж��ַ����еĸ����Ƿ�Ϸ�
  if ( (bWithSign==true) && (JudgeSignDOT(strtemp,"-") == false) )
  {
    strcpy(strDst,""); return;
  }

  // �ж��ַ����е������Ƿ�Ϸ�
  if ( (bWithSign==true) && (JudgeSignDOT(strtemp,"+") == false) )
  {
    strcpy(strDst,""); return;
  }

  // �ж��ַ����е�Բ���Ƿ�Ϸ�
  if ( (bWithDOT==true) && (JudgeSignDOT(strtemp,".") == false) )
  {
    strcpy(strDst,""); return;
  }

  int iPosSrc,iPosDst,iLen;
  iPosSrc=iPosDst=iLen=0;

  iLen=strlen(strtemp);

  for (iPosSrc=0;iPosSrc<iLen;iPosSrc++)
  {
    if ( (bWithSign==true) && (strtemp[iPosSrc] == '+') )
    {
      strDst[iPosDst++]=strtemp[iPosSrc]; continue;
    }

    if ( (bWithSign==true) && (strtemp[iPosSrc] == '-') )
    {
      strDst[iPosDst++]=strtemp[iPosSrc]; continue;
    }

    if ( (bWithDOT==true) && (strtemp[iPosSrc] == '.') )
    {
      strDst[iPosDst++]=strtemp[iPosSrc]; continue;
    }

    if (isdigit(strtemp[iPosSrc])) strDst[iPosDst++]=strtemp[iPosSrc];
  }

  strDst[iPosDst]=0;

  return;
}

// �ж��ַ����еĸ��ź�Բ���Ƿ�Ϸ�
bool JudgeSignDOT(const char *strSrc,const char *strBZ)
{
  char *pos=0;
  pos=(char *)strstr(strSrc,strBZ);

  // ���û�а������������ַ������ͷ���true
  if (pos == 0) return true;

  // ���strlen(pos)==1����ʾ�����ֻ�з��ţ�û�������ַ�������false
  if (strlen(pos)==1) return false;

  // ������������ַ�����+�ţ���һ��Ҫ�ǵ�һ���ַ�
  if ( (strcmp(strBZ,"+") == 0) && (strncmp(strSrc,"+",1) != 0) ) return false;

  // ������������ַ�����-�ţ���һ��Ҫ�ǵ�һ���ַ�
  if ( (strcmp(strBZ,"-") == 0) && (strncmp(strSrc,"-",1) != 0) ) return false;

  // �������������������ַ������ͷ���false
  if (strstr(pos+1,strBZ) != NULL) return false;

  return true;
}

/*
  ��һ���ַ��������ʱ�����һ��ƫ�������õ�ƫ�ƺ��ʱ��
  in_stime�Ǵ����ʱ�䣬�����ʽ������һ��Ҫ����yyyymmddhh24miss���Ƿ��зָ���û�й�ϵ��
  ��yyyy-mm-dd hh24:mi:ssƫ��in_interval��
  �����ĸ�ʽ��fmt������fmtĿǰ��ȡֵ���£������Ҫ���������ӣ�
  yyyy-mm-dd hh24:mi:ss���˸�ʽ��ȱʡ��ʽ��
  yyyymmddhh24miss
  yyyymmddhh24miss
  yyyy-mm-dd
  yyyymmdd
  hh24:mi:ss
  hh24miss
  hh24:mi
  hh24mi
  ����ֵ��0-�ɹ���-1-ʧ�ܡ�
*/
int AddTime(const char *in_stime,char *out_stime,const int in_interval,const char *in_fmt)
{
  time_t  timer;
  struct tm nowtimer;

  timer=strtotime(in_stime)+in_interval;

  nowtimer = *localtime ( &timer ); nowtimer.tm_mon++;

  // Ϊ�˷�ֹin_stime��out_stimeΪͬһ���������������out_stime�ڴ˴���ʼ�������벻����ǰ
  out_stime[0]=0;

  if (in_fmt==0)
  {
    snprintf(out_stime,20,"%04u-%02u-%02u %02u:%02u:%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec); return 0;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd hh24:mi:ss") == 0)
  {
    snprintf(out_stime,20,"%04u-%02u-%02u %02u:%02u:%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec); return 0;
  }

  if (strcmp(in_fmt,"yyyymmddhh24miss") == 0)
  {
    snprintf(out_stime,15,"%04u%02u%02u%02u%02u%02u",nowtimer.tm_year+1900,
                    nowtimer.tm_mon,nowtimer.tm_mday,nowtimer.tm_hour,
                    nowtimer.tm_min,nowtimer.tm_sec); return 0;
  }

  if (strcmp(in_fmt,"yyyy-mm-dd") == 0)
  {
    snprintf(out_stime,11,"%04u-%02u-%02u",nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday); return 0;
  }
  if (strcmp(in_fmt,"yyyymmdd") == 0)
  {
    snprintf(out_stime,9,"%04u%02u%02u",nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday); return 0;
  }

  if (strcmp(in_fmt,"hh24:mi:ss") == 0)
  {
    snprintf(out_stime,9,"%02u:%02u:%02u",nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec); return 0;
  }

  if (strcmp(in_fmt,"hh24:mi") == 0)
  {
    snprintf(out_stime,9,"%02u:%02u",nowtimer.tm_hour,nowtimer.tm_min); return 0;
  }


  if (strcmp(in_fmt,"hh24mi") == 0)
  {
    snprintf(out_stime,7,"%02u%02u",nowtimer.tm_hour,nowtimer.tm_min); return 0;
  }

  return -1;
}

// ��ȡ�ļ���ʱ�䣬��modtime
void FileMTime(const char *in_FullFileName,char *out_ModTime)
{
  struct tm nowtimer;
  struct stat st_filestat;

  stat(in_FullFileName,&st_filestat);

  nowtimer = *localtime(&st_filestat.st_mtime);
  nowtimer.tm_mon++;

  snprintf(out_ModTime,15,"%04u%02u%02u%02u%02u%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);
}

// ����XMLBuffer�ĺ���
// in_XMLBuffer��XML��ʽ���ַ���
// in_FieldName���ֶεı�ǩ��
// out_Value����ȡ���ݴ�ŵı�����ָ��
bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,char *out_Value,const int in_Len)
{
  strcpy(out_Value,"");

  char *start=NULL,*end=NULL;
  char m_SFieldName[51],m_EFieldName[51];

  int m_NameLen = strlen(in_FieldName);
  memset(m_SFieldName,0,sizeof(m_SFieldName));
  memset(m_EFieldName,0,sizeof(m_EFieldName));

  snprintf(m_SFieldName,50,"<%s>",in_FieldName);
  snprintf(m_EFieldName,50,"</%s>",in_FieldName);

  start=0; end=0;

  start = (char *)strstr(in_XMLBuffer,m_SFieldName);

  if (start != 0)
  {
    end   = (char *)strstr(start,m_EFieldName);
  }

  if ((start==0) || (end == 0))
  {
    return false;
  }

  int   m_ValueLen = end - start - m_NameLen - 2 + 1 ;

  if ( ((m_ValueLen-1) <= in_Len) || (in_Len == 0) )
  {
    strncpy(out_Value,start+m_NameLen+2,m_ValueLen-1); out_Value[m_ValueLen-1]=0;
  }
  else
  {
    strncpy(out_Value,start+m_NameLen+2,in_Len); out_Value[in_Len]=0;
  }

  DeleteLRChar(out_Value,' ');

  return true;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,bool *out_Value)
{
  (*out_Value) = false;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,10) == true)
  {
    if ( (strcmp(strTemp,"true")==0) || (strcmp(strTemp,"true")==0) ) (*out_Value)=true; return true;
  }

  return false;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,int *out_Value)
{
  (*out_Value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,50) == true)
  {
    (*out_Value) = atoi(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,unsigned int *out_Value)
{
  (*out_Value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,50) == true)
  {
    (*out_Value) = (unsigned int)atoi(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,long *out_Value)
{
  (*out_Value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,50) == true)
  {
    (*out_Value) = atol(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,unsigned long *out_Value)
{
  (*out_Value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,50) == true)
  {
    (*out_Value) = (unsigned long)atol(strTemp); return true;
  }

  return false;
}

bool GetXMLBuffer(const char *in_XMLBuffer,const char *in_FieldName,double *out_Value)
{
  (*out_Value) = 0;

  char strTemp[51];

  memset(strTemp,0,sizeof(strTemp));

  if (GetXMLBuffer(in_XMLBuffer,in_FieldName,strTemp,50) == true)
  {
    (*out_Value) = atof(strTemp); return true;
  }

  return false;
}

// �ж��ļ����Ƿ��MatchFileNameƥ�䣬�����ƥ�䣬����ʧ��
bool MatchFileName(const string in_FileName,const string in_MatchStr)
{
  // ������ڱȽϵ��ַ��ǿյģ�����false
  if (in_MatchStr.size() == 0) return false;

  // ������Ƚϵ��ַ����ǡ�*��������true
  if (in_MatchStr == "*") return true;

  // �����ļ���ƥ������е�ʱ��ƥ��dd-nn.mm
  char strTemp[2049];
  memset(strTemp,0,sizeof(strTemp));
  strncpy(strTemp,in_MatchStr.c_str(),2000);

  int ii,jj;
  int  iPOS1,iPOS2;
  CCmdStr CmdStr,CmdSubStr;

  string strFileName,strMatchStr;

  strFileName=in_FileName;
  strMatchStr=strTemp;

  // ���ַ�����ת���ɴ�д�������Ƚ�
  ToUpper(strFileName);
  ToUpper(strMatchStr);

  CmdStr.SplitToCmd(strMatchStr,",");

  for (ii=0;ii<CmdStr.CmdCount();ii++)
  {
    // ���Ϊ�գ���һ��Ҫ����������ͻᱻ����
    if (CmdStr.m_vCmdStr[ii].empty() == true) continue;

    iPOS1=iPOS2=0;
    CmdSubStr.SplitToCmd(CmdStr.m_vCmdStr[ii],"*");

    for (jj=0;jj<CmdSubStr.CmdCount();jj++)
    {
      // ������ļ������ײ�
      if (jj == 0)
      {
        if (strncmp(strFileName.c_str(),CmdSubStr.m_vCmdStr[jj].c_str(),CmdSubStr.m_vCmdStr[jj].size()) != 0) break;
      }

      // ������ļ�����β��
      if (jj == CmdSubStr.CmdCount()-1)
      {
        if (strcmp(strFileName.c_str()+strFileName.size()-CmdSubStr.m_vCmdStr[jj].size(),CmdSubStr.m_vCmdStr[jj].c_str()) != 0) break;
      }

      iPOS2=strFileName.find(CmdSubStr.m_vCmdStr[jj],iPOS1);

      if (iPOS2 < 0) break;

      iPOS1=iPOS2+CmdSubStr.m_vCmdStr[jj].size();
    }

    if (jj==CmdSubStr.CmdCount()) return true;
  }

  return false;
}

void ToUpper(char *str)
{
  if (str == 0) return;

  if (strlen(str) == 0) return;

  int istrlen=strlen(str);

  for (int ii=0;ii<istrlen;ii++)
  {
    if ( (str[ii] >= 97) && (str[ii] <= 122) ) str[ii]=str[ii] - 32;
  }
}

void ToUpper(string &str)
{
  if (str.empty()) return;

  char strtemp[str.size()+1];

  memset(strtemp,0,sizeof(strtemp));
  strcpy(strtemp,str.c_str());

  ToUpper(strtemp);

  str=strtemp;

  return;
}

void ToLower(char *str)
{
  if (str == 0) return;

  if (strlen(str) == 0) return;

  int istrlen=strlen(str);

  for (int ii=0;ii<istrlen;ii++)
  {
    if ( (str[ii] >= 65) && (str[ii] <= 90) ) str[ii]=str[ii] + 32;
  }
}

void ToLower(string &str)
{
  if (str.empty()) return;

  char strtemp[str.size()+1];

  memset(strtemp,0,sizeof(strtemp));
  strcpy(strtemp,str.c_str());

  ToLower(strtemp);

  str=strtemp;

  return;
}


CDir::CDir()
{
  m_uPOS=0;

  memset(m_DateFMT,0,sizeof(m_DateFMT));
  strcpy(m_DateFMT,"yyyy-mm-dd hh24:mi:ss");

  m_vFileName.clear();

  initdata();
}

void CDir::initdata()
{
  memset(m_DirName,0,sizeof(m_DirName));
  memset(m_FileName,0,sizeof(m_FileName));
  memset(m_FullFileName,0,sizeof(m_FullFileName));
  m_FileSize=0;
  memset(m_CreateTime,0,sizeof(m_CreateTime));
  memset(m_ModifyTime,0,sizeof(m_ModifyTime));
  memset(m_AccessTime,0,sizeof(m_AccessTime));
}

// ��������ʱ��ĸ�ʽ��֧��"yyyy-mm-dd hh24:mi:ss"��"yyyymmddhh24miss"���ָ�ʽ��ȱʡ��ǰ��
void CDir::SetDateFMT(const char *in_DateFMT)
{
  memset(m_DateFMT,0,sizeof(m_DateFMT));
  strcpy(m_DateFMT,in_DateFMT);
}

// ��Ŀ¼����ȡ�ļ�����Ϣ�������m_vFileName������
// in_dirname�����򿪵�Ŀ¼��
// in_MatchStr������ȡ�ļ�����ƥ�����
// in_MaxCount����ȡ�ļ����������
// bAndChild���Ƿ�򿪸�����Ŀ¼
// bSort���Ƿ�Խ��ʱ������
bool CDir::OpenDir(const char *in_DirName,const char *in_MatchStr,const unsigned int in_MaxCount,const bool bAndChild,bool bSort)
{
  m_uPOS=0;
  m_vFileName.clear();

  // ���Ŀ¼�����ڣ��ʹ�����Ŀ¼
  if (MKDIR(in_DirName,false) == false) return false;

  bool bRet=_OpenDir(in_DirName,in_MatchStr,in_MaxCount,bAndChild);

  if (bSort==true)
  {
    sort(m_vFileName.begin(), m_vFileName.end());
  }

  return bRet;
}

// ��Ŀ¼�����Ǹ��ݹ麯��
bool CDir::_OpenDir(const char *in_DirName,const char *in_MatchStr,const unsigned int in_MaxCount,const bool bAndChild)
{
  DIR *dir;

  if ( (dir=opendir(in_DirName)) == NULL ) return false;

  char strTempFileName[1024];

  struct dirent *st_fileinfo;
  struct stat st_filestat;

  while ((st_fileinfo=readdir(dir)) != NULL)
  {
    // ��"."��ͷ���ļ�������
    if (st_fileinfo->d_name[0]=='.') continue;
        
    memset(strTempFileName,0,sizeof(strTempFileName));

    snprintf(strTempFileName,300,"%s//%s",in_DirName,st_fileinfo->d_name);

    UpdateStr(strTempFileName,"//","/");

    stat(strTempFileName,&st_filestat);

    // �ж��Ƿ���Ŀ¼
    if (S_ISDIR(st_filestat.st_mode))
    {
      if (bAndChild == true)
      {
        if (_OpenDir(strTempFileName,in_MatchStr,in_MaxCount,bAndChild) == false) 
        {
          closedir(dir); return false;
        }
      }
    }
    else
    {
      if (MatchFileName(st_fileinfo->d_name,in_MatchStr) == false) continue;

      m_vFileName.push_back(strTempFileName);

      if ( m_vFileName.size()>in_MaxCount ) break;
    }
  }

  closedir(dir);

  return true;
}

/*
st_gid 

Numeric identifier of group that owns file (UNIX-specific) This field will always be zero on NT systems. A redirected file is classified as an NT file.

st_atime

Time of last access of file.

st_ctime

Time of creation of file.

st_dev

Drive number of the disk containing the file (same as st_rdev).

st_ino

Number of the information node (the inode) for the file (UNIX-specific). On UNIX file systems, the inode describes the file date and time stamps, permissions, and content. When files are hard-linked to one another, they share the same inode. The inode, and therefore st_ino, has no meaning in the FAT, HPFS, or NTFS file systems.

st_mode

Bit mask for file-mode information. The _S_IFDIR bit is set if path specifies a directory; the _S_IFREG bit is set if path specifies an ordinary file or a device. User read/write bits are set according to the file��s permission mode; user execute bits are set according to the filename extension.

st_mtime

Time of last modification of file.

st_nlink

Always 1 on non-NTFS file systems.

st_rdev

Drive number of the disk containing the file (same as st_dev).

st_size

Size of the file in bytes; a 64-bit integer for _stati64 and _wstati64

st_uid

Numeric identifier of user who owns file (UNIX-specific). This field will always be zero on NT systems. A redirected file is classified as an NT file.
*/

bool CDir::ReadDir()
{
  initdata();

  // ����Ѷ��꣬�������
  if (m_uPOS >= m_vFileName.size()) 
  {
    m_uPOS=0; m_vFileName.clear(); return false;
  }

  int pos=0;

  pos=m_vFileName[m_uPOS].find_last_of("/");

  // Ŀ¼��
  memset(m_DirName,0,sizeof(m_DirName));
  strcpy(m_DirName,m_vFileName[m_uPOS].substr(0,pos).c_str());

  // �ļ���
  memset(m_FileName,0,sizeof(m_FileName));
  strcpy(m_FileName,m_vFileName[m_uPOS].substr(pos+1,m_vFileName[m_uPOS].size()-pos-1).c_str());

  // �ļ�ȫ��������·��
  snprintf(m_FullFileName,300,"%s",m_vFileName[m_uPOS].c_str());

  struct stat st_filestat;

  stat(m_FullFileName,&st_filestat);

  m_FileSize=st_filestat.st_size;

  struct tm nowtimer;

  if (strcmp(m_DateFMT,"yyyy-mm-dd hh24:mi:ss") == 0)
  {
    nowtimer = *localtime(&st_filestat.st_mtime); nowtimer.tm_mon++;
    snprintf(m_ModifyTime,20,"%04u-%02u-%02u %02u:%02u:%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_ctime); nowtimer.tm_mon++;
    snprintf(m_CreateTime,20,"%04u-%02u-%02u %02u:%02u:%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_atime); nowtimer.tm_mon++;
    snprintf(m_AccessTime,20,"%04u-%02u-%02u %02u:%02u:%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);
  }

  if (strcmp(m_DateFMT,"yyyymmddhh24miss") == 0)
  {
    nowtimer = *localtime(&st_filestat.st_mtime); nowtimer.tm_mon++;
    snprintf(m_ModifyTime,20,"%04u%02u%02u%02u%02u%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_ctime); nowtimer.tm_mon++;
    snprintf(m_CreateTime,20,"%04u%02u%02u%02u%02u%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);

    nowtimer = *localtime(&st_filestat.st_atime); nowtimer.tm_mon++;
    snprintf(m_AccessTime,20,"%04u%02u%02u%02u%02u%02u",\
             nowtimer.tm_year+1900,nowtimer.tm_mon,nowtimer.tm_mday,\
             nowtimer.tm_hour,nowtimer.tm_min,nowtimer.tm_sec);
  }

  m_uPOS++;

  return true;
}

CDir::~CDir()
{
  m_vFileName.clear();

  // m_vDirName.clear();
}

// ���ַ����е�ĳ�ַ�������һ���ַ�������
void UpdateStr(char *in_string,const char *in_str1,const char *in_str2,bool bLoop)
{
  if (in_string == 0) return;

  if (strlen(in_string) == 0) return;

  char strTemp[2048];

  char *strStart=in_string;

  char *strPos=0;

  while (true)
  {
    if (strlen(in_string) >2000) break;

    if (bLoop == true)
    {
      strPos=strstr(in_string,in_str1);
    }
    else
    {
      strPos=strstr(strStart,in_str1);
    }

    if (strPos == 0) break;

    memset(strTemp,0,sizeof(strTemp));
    strncpy(strTemp,in_string,strPos-in_string);
    strcat(strTemp,in_str2);
    strcat(strTemp,strPos+strlen(in_str1));
    strcpy(in_string,strTemp);

    strStart=strPos+strlen(in_str2);
  }
}

// ɾ���ļ������ɾ��ʧ�ܣ��᳢��in_times��
bool REMOVE(const char *in_filename,const int in_times)
{
  // ����ļ������ڣ�ֱ�ӷ���ʧ��
  if (access(in_filename,R_OK) != 0) return false;

  for (int ii=0;ii<in_times;ii++)
  {
    if (remove(in_filename) == 0) return true;

    usleep(100000);
  }

  return false;
}

// ��in_srcfilename����Ϊin_dstfilename���������ʧ�ܣ��᳢��in_times��
bool RENAME(const char *in_srcfilename,const char *in_dstfilename,const int in_times)
{
  // ����ļ������ڣ�ֱ�ӷ���ʧ��
  if (access(in_srcfilename,R_OK) != 0) return false;

  if (MKDIR(in_dstfilename) == false) return false;

  for (int ii=0;ii<in_times;ii++)
  {
    if (rename(in_srcfilename,in_dstfilename) == 0) return true;

    usleep(100000);
  }

  return false;
}


CTcpClient::CTcpClient()
{
  m_sockfd=-1;
  memset(m_ip,0,sizeof(m_ip));
  m_port=0;
  m_btimeout=false;
}

bool CTcpClient::ConnectToServer(const char *in_ip,const int in_port)
{
  if (m_sockfd != -1) { close(m_sockfd); m_sockfd = -1; }

  strcpy(m_ip,in_ip);
  m_port=in_port;

  struct hostent* h;
  struct sockaddr_in servaddr;

  if ( (m_sockfd = socket(AF_INET,SOCK_STREAM,0) ) < 0) return false;

  if ( !(h = gethostbyname(m_ip)) )
  {
    close(m_sockfd);  m_sockfd = -1; return false;
  }

  memset(&servaddr,0,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(m_port);  // ָ������˵�ͨѶ�˿�
  memcpy(&servaddr.sin_addr,h->h_addr,h->h_length);

  if (connect(m_sockfd, (struct sockaddr *)&servaddr,sizeof(servaddr)) != 0)
  {
    close(m_sockfd);  m_sockfd = -1; return false;
  }

  return true;
}

bool CTcpClient::Read(char *strRecvBuffer)
{
  if (m_sockfd == -1) return false;

  m_buflen = 0;
  return(TcpRead(m_sockfd,strRecvBuffer,&m_buflen));
}

bool CTcpClient::Read(char *strRecvBuffer,const int iTimeOut)
{
  if (m_sockfd == -1) return false;

  fd_set tmpfd;

  FD_ZERO(&tmpfd);
  FD_SET(m_sockfd,&tmpfd);

  struct timeval timeout;
  timeout.tv_sec = iTimeOut; timeout.tv_usec = 0;

  m_btimeout = false;

  int i;
  if ( (i = select(m_sockfd+1,&tmpfd,NULL,NULL,&timeout)) <= 0 )
  {
    if (i==0) m_btimeout = true;
    return false;
  }

  m_buflen = 0;
  return(TcpRead(m_sockfd,strRecvBuffer,&m_buflen));
}

bool CTcpClient::Write(char *strSendBuffer)
{
  if (m_sockfd == -1) return false;

  int buflen = strlen(strSendBuffer);

  return(Write(strSendBuffer,buflen));
}

bool CTcpClient::Write(char *strSendBuffer,const int buflen)
{
  if (m_sockfd == -1) return false;

  fd_set tmpfd;

  FD_ZERO(&tmpfd);
  FD_SET(m_sockfd,&tmpfd);

  struct timeval timeout;
  timeout.tv_sec = 5; timeout.tv_usec = 0;
  
  m_btimeout = false;

  int i;
  if ( (i=select(m_sockfd+1,NULL,&tmpfd,NULL,&timeout)) <= 0 )
  {
    if (i==0) m_btimeout = true;
    return false;
  }

  return(TcpWrite(m_sockfd,strSendBuffer,buflen));
}

void CTcpClient::Close()
{
  if (m_sockfd > 0)
  {
    close(m_sockfd); m_sockfd=-1;
  }
}

CTcpClient::~CTcpClient()
{
  Close();
}

CTcpServer::CTcpServer()
{
  m_listenfd=-1;
  m_connfd=-1;
  m_socklen=0;
  m_btimeout=false;
}

bool CTcpServer::InitServer(const unsigned int port)
{
  if (m_listenfd > 0) { close(m_listenfd); m_listenfd=-1; }

  m_listenfd = socket(AF_INET,SOCK_STREAM,0);

  // WINDOWSƽ̨����
  //char b_opt='1';
  //setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&b_opt,sizeof(b_opt));
  //setsockopt(m_listenfd,SOL_SOCKET,SO_KEEPALIVE,&b_opt,sizeof(b_opt));

  // Linux����
  int opt = 1; unsigned int len = sizeof(opt);
  setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,len);
  setsockopt(m_listenfd,SOL_SOCKET,SO_KEEPALIVE,&opt,len);

  memset(&m_servaddr,0,sizeof(m_servaddr));
  m_servaddr.sin_family = AF_INET;
  m_servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  m_servaddr.sin_port = htons(port);
  if (bind(m_listenfd,(struct sockaddr *)&m_servaddr,sizeof(m_servaddr)) != 0 )
  {
    CloseListen(); return false;
  }

  if (listen(m_listenfd,5) != 0 )
  {
    CloseListen(); return false;
  }

  m_socklen = sizeof(struct sockaddr_in);

  return true;
}

bool CTcpServer::Accept()
{
  if (m_listenfd == -1) return false;

  if ((m_connfd=accept(m_listenfd,(struct sockaddr *)&m_clientaddr,(socklen_t*)&m_socklen)) < 0)
      return false;

  return true;
}

char *CTcpServer::GetIP()
{
  return(inet_ntoa(m_clientaddr.sin_addr));
}

bool CTcpServer::Read(char *strRecvBuffer)
{
  if (m_connfd == -1) return false;

  m_buflen = 0;
  return(TcpRead(m_connfd,strRecvBuffer,&m_buflen));
}

bool CTcpServer::Read(char *strRecvBuffer,const int iTimeOut)
{
  if (m_connfd == -1) return false;

  fd_set tmpfd;

  FD_ZERO(&tmpfd);
  FD_SET(m_connfd,&tmpfd);

  struct timeval timeout;
  timeout.tv_sec = iTimeOut; timeout.tv_usec = 0;

  m_btimeout = false;

  int i;
  if ( (i = select(m_connfd+1,&tmpfd,NULL,NULL,&timeout)) <= 0 )
  {
    if (i==0) m_btimeout = true;
    return false;
  }

  m_buflen = 0;
  return(TcpRead(m_connfd,strRecvBuffer,&m_buflen));
}

bool CTcpServer::Write(char *strSendBuffer)
{
  if (m_connfd == -1) return false;

  int buflen = strlen(strSendBuffer);

  return(Write(strSendBuffer,buflen));
}

bool CTcpServer::Write(char *strSendBuffer,const int buflen)
{
  if (m_connfd == -1) return false;

  fd_set tmpfd;

  FD_ZERO(&tmpfd);
  FD_SET(m_connfd,&tmpfd);

  struct timeval timeout;
  timeout.tv_sec = 5; timeout.tv_usec = 0;
  
  m_btimeout = false;

  int i;
  if ( (i=select(m_connfd+1,NULL,&tmpfd,NULL,&timeout)) <= 0 )
  {
    if (i==0) m_btimeout = true;
    return false;
  }

  return(TcpWrite(m_connfd,strSendBuffer,buflen));
}

void CTcpServer::CloseListen()
{
  if (m_listenfd > 0)
  {
    close(m_listenfd); m_listenfd=-1;
  }
}

void CTcpServer::CloseClient()
{
  if (m_connfd > 0)
  {
    close(m_connfd); m_connfd=-1; 
  }
}

CTcpServer::~CTcpServer()
{
  CloseListen(); CloseClient();
}

bool TcpRead(const int fd,char *strRecvBuffer,int *buflen,const int iTimeOut)
{
  if (fd == -1) return false;

  if (iTimeOut > 0)
  {
    fd_set tmpfd;

    FD_ZERO(&tmpfd);
    FD_SET(fd,&tmpfd);

    struct timeval timeout;
    timeout.tv_sec = iTimeOut; timeout.tv_usec = 0;

    int i;
    if ( (i = select(fd+1,&tmpfd,NULL,NULL,&timeout)) <= 0 ) return false;
  }

  (*buflen) = 0;

  char strBufLen[TCPHEADLEN+1]; memset(strBufLen,0,sizeof(strBufLen));

  if (Readn(fd,(char*)strBufLen,TCPHEADLEN) == false) return false;

  (*buflen) = atoi(strBufLen);

  if ( (*buflen) > TCPBUFLEN ) return false;

  if (Readn(fd,strRecvBuffer,(*buflen)) == false) return false;

  return true;
}

bool TcpWrite(const int fd,const char *strSendBuffer,const int buflen,const int iTimeOut)
{
  if (fd == -1) return false;

  if (iTimeOut > 0)
  {
    fd_set tmpfd;

    FD_ZERO(&tmpfd);
    FD_SET(fd,&tmpfd);

    struct timeval timeout;
    timeout.tv_sec = 5; timeout.tv_usec = 0;

    if ( select(fd+1,NULL,&tmpfd,NULL,&timeout) <= 0 ) return false;
  }
  
  int ibuflen=0;

  // �������Ϊ0���Ͳ����ַ����ĳ���
  if (buflen==0) ibuflen=strlen(strSendBuffer);
  else ibuflen=buflen;

  if (ibuflen>TCPBUFLEN) return false;

  char strBufLen[TCPHEADLEN+1]; 
  memset(strBufLen,0,sizeof(strBufLen));
  sprintf(strBufLen,"%d",ibuflen);

  char strTBuffer[TCPHEADLEN+ibuflen];
  memset(strTBuffer,0,sizeof(strTBuffer));
  memcpy(strTBuffer,strBufLen,TCPHEADLEN);
  memcpy(strTBuffer+TCPHEADLEN,strSendBuffer,ibuflen);

  if (Writen(fd,strTBuffer,TCPHEADLEN+ibuflen) == false) return false;

  return true;
}

bool Readn(const int fd,char *vptr,const size_t n)
{
  int nLeft,nread,idx;

  nLeft = n;
  idx = 0;

  while(nLeft > 0)
  {
    if ( (nread = recv(fd,vptr + idx,nLeft,0)) <= 0) return false;

    idx += nread;
    nLeft -= nread;
  }

  return true;
}

bool Writen(const int fd,const char *vptr,const size_t n)
{
  int nLeft,idx,nwritten;
  nLeft = n;  
  idx = 0;
  while(nLeft > 0 )
  {    
    if ( (nwritten = send(fd, vptr + idx,nLeft,0)) <= 0) return false;      
    
    nLeft -= nwritten;
    idx += nwritten;
  }

  return true;
}


// ���ļ�ͨ��sockfd���͸��Զ�
bool SendFile(CLogFile *logfile,int sockfd,struct st_fileinfo *stfileinfo)
{
  char strSendBuffer[301],strRecvBuffer[301];
  memset(strSendBuffer,0,sizeof(strSendBuffer));

  snprintf(strSendBuffer,300,"<filename>%s</filename><filesize>%d</filesize><mtime>%s</mtime>",stfileinfo->filename,stfileinfo->filesize,stfileinfo->mtime);

  // logfile->Write("SendFile strSendBuffer=%s\n",strSendBuffer);  // xxxxxx
  if (TcpWrite(sockfd,strSendBuffer) == false)
  {
    logfile->Write("SendFile TcpWrite() failed.\n"); return false;
  }

  int  bytes=0;
  int  total_bytes=0;
  int  onread=0;
  char buffer[1000];

  FILE *fp=NULL;

  if ( (fp=FOPEN(stfileinfo->filename,"rb")) == NULL )
  {
    logfile->Write("SendFile FOPEN(%s) failed.\n",stfileinfo->filename); return false;
  }

  while (true)
  {
    memset(buffer,0,sizeof(buffer));

    if ((stfileinfo->filesize-total_bytes) > 1000) onread=1000;
    else onread=stfileinfo->filesize-total_bytes;

    bytes=fread(buffer,1,onread,fp);

    if (bytes > 0)
    {
      if (Writen(sockfd,buffer,bytes) == false)
      {
        logfile->Write("SendFile Writen() failed.\n"); fclose(fp); fp=NULL; return false;
      }
    }

    total_bytes = total_bytes + bytes;

    if ((int)total_bytes == stfileinfo->filesize) break;
  }

  fclose(fp);

  // ���նԶ˷��ص�ȷ�ϱ���
  int buflen=0;
  memset(strRecvBuffer,0,sizeof(strRecvBuffer));
  if (TcpRead(sockfd,strRecvBuffer,&buflen)==false)
  {
    logfile->Write("SendFile TcpRead() failed.\n"); return false;
  }
  // logfile->Write("SendFile strRecvBuffer=%s\n",strRecvBuffer);  // xxxxxx

  if (strcmp(strRecvBuffer,"ok")!=0) return false;

  return true;
}

// ����ͨ��socdfd���͹������ļ�
bool RecvFile(CLogFile *logfile,int sockfd,struct st_fileinfo *stfileinfo)
{
  char strSendBuffer[301],strRecvBuffer[301];

  char strfilenametmp[301]; memset(strfilenametmp,0,sizeof(strfilenametmp));
  sprintf(strfilenametmp,"%s.tmp",stfileinfo->filename);

  FILE *fp=NULL;

  if ( (fp=FOPEN(strfilenametmp,"wb")) ==NULL)     // FOPEN�ɴ���Ŀ¼
  {
    logfile->Write("RecvFile FOPEN %s failed.\n",strfilenametmp); return false;
  }

  int  total_bytes=0;
  int  onread=0;
  char buffer[1000];

  while (true)
  {
    memset(buffer,0,sizeof(buffer));

    if ((stfileinfo->filesize-total_bytes) > 1000) onread=1000;
    else onread=stfileinfo->filesize-total_bytes;

    if (Readn(sockfd,buffer,onread) == false)
    {
      logfile->Write("RecvFile Readn() failed.\n"); fclose(fp); fp=NULL; return false;
    }

    fwrite(buffer,1,onread,fp);

    total_bytes = total_bytes + onread;

    if ((int)total_bytes == stfileinfo->filesize) break;
  }

  fclose(fp);

  // �����ļ���ʱ��
  UTime(strfilenametmp,stfileinfo->mtime);

  memset(strSendBuffer,0,sizeof(strSendBuffer));
  // logfile->Write("strfilenametmp=%s\n",strfilenametmp);
  if (RENAME(strfilenametmp,stfileinfo->filename)==true) strcpy(strSendBuffer,"ok");
  else strcpy(strSendBuffer,"failed");
  // logfile->Write("filename=%s\n",stfileinfo->filename);

  // ��Զ˷�����Ӧ����
  // logfile->Write("RecvFile strSendBuffer=%s\n",strSendBuffer);  // xxxxxx
  if (TcpWrite(sockfd,strSendBuffer)==false)
  {
    logfile->Write("RecvFile TcpWrite() failed.\n"); return false;
  }

  if (strcmp(strSendBuffer,"ok") != 0) return false;

  return true;
}

// ��ĳһ���ļ����Ƶ���һ���ļ�
bool COPY(const char *srcfilename,const char *dstfilename)
{
  if (MKDIR(dstfilename) == false) return false;

  char strdstfilenametmp[301];
  memset(strdstfilenametmp,0,sizeof(strdstfilenametmp));
  snprintf(strdstfilenametmp,300,"%s.tmp",dstfilename);

  int  srcfd,dstfd;

  srcfd=dstfd=-1;

  int iFileSize=FileSize(srcfilename);

  int  bytes=0;
  int  total_bytes=0;
  int  onread=0;
  char buffer[5000];

  if ( (srcfd=open(srcfilename,O_RDONLY)) < 0 ) return false;

  if ( (dstfd=open(strdstfilenametmp,O_WRONLY|O_CREAT|O_TRUNC,S_IWUSR|S_IRUSR|S_IXUSR)) < 0) { close(srcfd); return false; }

  while (true)
  {
    memset(buffer,0,sizeof(buffer));

    if ((iFileSize-total_bytes) > 5000) onread=5000;
    else onread=iFileSize-total_bytes;

    bytes=read(srcfd,buffer,onread);

    if (bytes > 0) write(dstfd,buffer,bytes);

    total_bytes = total_bytes + bytes;

    if (total_bytes == iFileSize) break;
  }

  close(srcfd);

  close(dstfd);

  // �����ļ����޸�ʱ������
  char strmtime[21];
  memset(strmtime,0,sizeof(strmtime));
  FileMTime(srcfilename,strmtime);
  UTime(strdstfilenametmp,strmtime);

  if (RENAME(strdstfilenametmp,dstfilename) == false) { REMOVE(strdstfilenametmp); return false; }

  return true;
}


CTimer::CTimer()
{
  memset(&m_start,0,sizeof(struct timeval));
  memset(&m_end,0,sizeof(struct timeval));

  // ��ʼ��ʱ
  Start();
}

// ��ʼ��ʱ
void CTimer::Start()
{
  gettimeofday( &m_start, NULL );
}

// ��������ȥ��ʱ�䣬��λ���룬С���������΢��
double CTimer::Elapsed()
{

  gettimeofday( &m_end, NULL );

  double dstart,dend;

  dstart=dend=0;

  char strtemp[51];
  memset(strtemp,0,sizeof(strtemp));
  snprintf(strtemp,30,"%ld.%ld",m_start.tv_sec,m_start.tv_usec);
  dstart=atof(strtemp);

  memset(strtemp,0,sizeof(strtemp));
  snprintf(strtemp,30,"%ld.%ld",m_end.tv_sec,m_end.tv_usec);
  dend=atof(strtemp);

  // ���¿�ʼ��ʱ
  Start();

  return dend-dstart;
}

