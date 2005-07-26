//*****************************************************************************
// fat.cpp :
// 2004/02/02 by Gaku :
// Licence : see http://gaku.s12.xrea.com/wiki/main.cgi?c=g&p=Gaku%2FLicence
//*****************************************************************************
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#ifndef MONA
#include <stdio.h>	
#include <stdlib.h>
#include <string.h>
#endif

#include "fat.h"
using namespace FatFS;
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FAT
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//-----------------------------------------------------------------------------
const byte FAT::FileSystemID[] = "FAT12   ";

//=============================================================================
FAT::FAT ()
{
	fat = NULL;
}

//=============================================================================
FAT::~FAT ()
{
	delete[] fat;
}

//=============================================================================
bool FAT::initialize (IStorageDevice *p)
{
	byte bf[SECTOR_SIZE];

	bytesPerSector = SECTOR_SIZE;
	floppy = p;
	if (false == read(0, bf))
		return false;

	// File System ID ���m�F����
	if (0 != memcmp(bf + FILE_SYSTEM_ID, FileSystemID, 8))
		return false;

	// BIOS Parameter Block �̏���ǂݍ���
	dword numberOfSectors;

	bytesPerSector    = *((word*)( bf + BYTES_PER_SECTOR    ));
	sectorsPerCluster = *((byte*)( bf + SECTORS_PER_CLUSTER ));
	reservedSectors   = *((word*)( bf + RESERVED_SECTORS    ));
	numberOfFats      = *((byte*)( bf + NUMBER_OF_FATS      ));
	numberOfDirEntry  = *((word*)( bf + NUMBER_OF_DIRENTRY  ));
	numberOfSectors   = *((word*)( bf + NUMBER_OF_SECTORS   ));
	sectorsPerFat     = *((word*)( bf + SECTORS_PER_FAT     ));

	dword bytesPerFat = bytesPerSector * sectorsPerFat;
	dword sectorsPerDirEntry = numberOfDirEntry / ( bytesPerSector / 0x20 );

	rootDirectoryEntry = reservedSectors + sectorsPerFat * numberOfFats;
	dataArea = rootDirectoryEntry + sectorsPerDirEntry;
	numberOfClusters = numberOfSectors - dataArea;

	// �������m��
	byte *ptr = new byte [bytesPerFat + sectorsPerFat];
	if (NULL == ptr)
		return false;

	// File Allocation Table �̓ǂݍ���
	byte *tmp = ptr;
	for (dword n = 0; n < sectorsPerFat; n++) {
		if (false == read(reservedSectors + n, tmp)) {
			delete[] ptr;
			return false;
		}
		tmp += bytesPerSector;
	}

	fat = ptr;
	flag = ptr + bytesPerFat;

	return true;
}

//=============================================================================
dword FAT::getBytesPerSector ()
{
	return bytesPerSector;
}

//=============================================================================
dword FAT::getSectorsPerCluster ()
{
	return sectorsPerCluster;
}

//=============================================================================
dword FAT::getNumberOfDirEntry ()
{
	return numberOfDirEntry;
}

//=============================================================================
dword FAT::getNumberOfClusters ()
{
	return numberOfClusters;
}

//=============================================================================
dword FAT::getRootDirectoryEntry ()
{
	return rootDirectoryEntry;
}

//=============================================================================
byte* FAT::readSectors (dword c, dword s, dword d, dword *sects, dword *last)
{
	dword num = getSectorsPerCluster();
	dword bsize = getBytesPerSector();

	dword *lba = new dword [s];
	if (NULL == lba)
		return NULL;

	dword i = 0;
	dword l = 0;

	// FAT �̓ǂݍ���
	while (getNumberOfClusters() > c) {
		dword base = getLbaFromCluster(c);

		if (s < i + num) {
			// �T�C�Y������Ȃ��Ȃ����̂Ń������̊g��
			dword tmp_size = s + d;
			dword *tmp = new dword [tmp_size];
			if (NULL == tmp) {
				delete[] lba;
				return NULL;
			}

			memcpy(tmp, lba, s * sizeof(dword));

			delete[] lba;

			lba = tmp;
			s = tmp_size;
		}

		for (dword n = 0; n < num; n++) 
			lba[i++] = base+n;

		l = c;
		c = getNextCluster(c);
	}

	// �Z�N�^�̓ǂݍ���
	// �Z�N�^�{�� + �Z�N�^�C���f�b�N�X + �Z�N�^����t���O
	dword size = bsize * i + sizeof(dword) * i + i;

	byte *ptr = new byte [size];
	if (NULL == ptr) {
		delete[] lba;
		return NULL;
	}

	byte *tmp = ptr;

	for (dword n = 0; n < i; n++) {
		if (false == read(lba[n], tmp)) {
			delete[] lba;
			delete[] ptr;
			return NULL;
		}
		tmp += bsize;
	}

	memcpy(tmp, lba, i * sizeof(dword));

	delete[] lba;

	*sects = i;
	*last = l;

	return ptr;
}

//=============================================================================
dword FAT::allocateCluster (dword cluster, dword count)
{
	dword next, temp;

	// �N���X�^�̐V�K�m��
	if (0 == cluster) {
		cluster = searchFreeCluster(START_OF_CLUSTER);
		if (0 == cluster)
			return 0;
		// �v�������[���ł��V�K�쐬�̏ꍇ�͍Œ��͊��蓖�Ă�
		if (0 < count)
			count--;
	}

	// �N���X�^��v�������������Ċm��
	next = cluster;
	while (0 < count--) {
		temp = searchFreeCluster(next+1);
		if (0 == temp) {
			freeCluster(cluster);
			return 0;
		}

		setNextCluster(next, temp);
		next = temp;
	}
	setNextCluster(next, END_OF_CLUSTER);

	// FAT �̏����߂�
	flushFat();

	return cluster;
}

//=============================================================================
void FAT::freeCluster (dword cluster)
{
	dword next;

	do {
		next = getNextCluster(cluster);
		setNextCluster(cluster, 0);
		cluster = next;
	} while (numberOfClusters > cluster);

	// FAT �̏����߂�
	flushFat();
}

//=============================================================================
void FAT::setEndOfCluster (dword cluster)
{
	setNextCluster(cluster, END_OF_CLUSTER);
}

//=============================================================================
bool FAT::read (dword lba, byte *bf)
{
	floppy->read(lba, bf, bytesPerSector);

	return true;
}

//=============================================================================
bool FAT::write (dword lba, byte *bf)
{
	floppy->write(lba, bf, bytesPerSector);

	return true;
}

//=============================================================================
dword FAT::getLbaFromCluster (dword cluster)
{
	return dataArea + sectorsPerCluster * ( cluster - 2 );
}

//=============================================================================
dword FAT::getClusterFromLba (dword lba)
{
	return ( lba - dataArea ) / sectorsPerCluster;
}

//=============================================================================
dword FAT::getNextCluster (dword cluster)
{
	dword index = cluster + cluster / 2;
	dword next;

	if (cluster & 0x01) {
		next = ( fat[index+1] << 4 ) + ( fat[index] >> 4 );
	} else {
		next = ( (fat[index+1] & 0x0f) << 8 ) + fat[index];
	}

	if (next == cluster)
		return END_OF_CLUSTER;

	return next;
}

//-----------------------------------------------------------------------------
void FAT::setNextCluster (dword cluster, dword next)
{
	dword index = cluster + cluster / 2;

	if (cluster & 0x01) {
		fat[index+1] = next >> 4;
		fat[index] = (fat[index] & 0x0f) | (next & 0x0f) << 4;
	} else {
		fat[index+1] = (fat[index+1] & 0xf0) | (next >> 8);
		fat[index] = next;
	}

	// �ύX�����Z�N�^�ɂ̓t���O�𗧂ĂĂ���
	flag[cluster * 3 / ( 2 * bytesPerSector ) ] = 1;
	flag[ ( cluster * 3 + 2 ) / ( 2 * bytesPerSector ) ] = 1;
}

//-----------------------------------------------------------------------------
dword FAT::searchFreeCluster (dword cluster)
{
	// �傫������N���X�^�ԍ���������A�����J�n�ʒu��擪�֖߂�
	if (numberOfClusters <= cluster)
		cluster = START_OF_CLUSTER;
	dword start = cluster;

	do {
		// �N���X�^���󂢂Ă��邩�𒲂ׂĂ���
		dword next = getNextCluster(cluster);
		if (0 == next)
			return cluster;
		cluster++;

		// �����܂ŗ�����A�擪���璲�ׂȂ���
		if (numberOfClusters <= cluster)
			cluster = START_OF_CLUSTER;
	} while (start != cluster);

	return 0;
}

//-----------------------------------------------------------------------------
void FAT::clearFlag ()
{
	for (dword n = 0; n < sectorsPerFat; n++) {
		flag[n] = 0;
	}
}

//-----------------------------------------------------------------------------
void FAT::flushFat ()
{
	// �t���O�������Ă���Z�N�^���f�B�X�N�ɏ����߂�
	for (dword n = 0; n < sectorsPerFat; n++) {
		if (0 != flag[n]) {
			byte *tmp = fat + n * bytesPerSector;
			write(reservedSectors + n, tmp);
			write(reservedSectors + sectorsPerFat + n, tmp);
			flag[n] = 0;
		}
	}
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FatFile
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//=============================================================================
FatFile::FatFile ()
{
	file = NULL;
}

//=============================================================================
FatFile::~FatFile ()
{
	flush();
	delete[] file;
}

//=============================================================================
bool FatFile::initialize (FAT *p, FatDirectory *d, int e, dword c, dword s)
{
	// �ǂݍ���
	dword bsize = p->getBytesPerSector();
	dword sects = 0;
	byte *ptr = NULL;
	last = 0;

	if (p->getNumberOfClusters() > c) {
		dword size = ( s + bsize - 1 ) / bsize;

		ptr = p->readSectors(c, size, RESIZE_DELTA, &sects, &last);
		if (NULL == ptr)
			return false;
	}

	// �e��p�����[�^�m��
	fat = p;
	parent = d;
	file = ptr;
	flag = ptr + bsize * sects + sizeof(dword) * sects;
	lba = (dword*)( ptr + bsize * sects );
	fsize = s;
	sectors = sects;
	pos = 0;
	entry = e;
	sizeChanged = false;

	// �t���O�N���A
	clearFlag();

	return true;
}

//=============================================================================
dword FatFile::read (byte *bf, dword sz)
{
	if (fsize < pos + sz)
		sz = fsize - pos;

	if (0 < sz) {
		memcpy(bf, file+pos, sz);
		pos += sz;
	}

	return sz;
}

//=============================================================================
dword FatFile::write (byte *bf, dword sz)
{
	if (fsize < pos + sz) {
		dword bytesPerSector = fat->getBytesPerSector();
		dword size = bytesPerSector * sectors;

		if (size < pos + sz) {
			// �N���X�^�̊m��
			if (false == expandClusters(sz))
				return 0;
		}

		fsize = pos + sz;
		sizeChanged = true;
	}

	if (0 < sz) {
		memcpy(file+pos, bf, sz);

		// �������񂾈ʒu�Ƀt���O�𗧂ĂĂ���
		dword bytesPerSector = fat->getBytesPerSector();
		dword n = pos / bytesPerSector;
		while (n <= (pos + sz-1) / bytesPerSector) {
			flag[n++] = 1;
		}

		pos += sz;
	}

	return sz;
}

//=============================================================================
bool FatFile::seek (int pt, int flag)
{
	if (SEEK_SET == flag) {
		pos = pt;
		if (0 > pt)
			pos = 0;
	} else if (SEEK_CUR == flag) {
		pos += pt;
		if (0 > pt) {
			if (pos < (dword)-pt)
				pos = 0;
		}
	} else if (SEEK_END == flag) {
		pos = fsize + pt;
		if (0 > pt) {
			if (fsize < (dword)-pt)
				pos = 0;
		}
	}

	if (pos > fsize)
		pos = fsize;

	return true;
}

//=============================================================================
bool FatFile::flush ()
{
	// �t���O�������Ă���Z�N�^���f�B�X�N�ɏ����߂�
	bool result = true;
	dword bytesPerSector = fat->getBytesPerSector();

	for (dword n = 0; n < sectors; n++) {
		if (0 != flag[n]) {
			result = fat->write(lba[n], file + n * bytesPerSector);
			flag[n] = 0;
		}
	}

	if (true == sizeChanged) {
		parent->setFileSize(entry, fsize);
		sizeChanged = false;
	}

	return result;
}

//=============================================================================
bool FatFile::resize (dword sz)
{
	if (fsize == sz)
		return true;

	dword bytesPerSector = fat->getBytesPerSector();
	dword size = bytesPerSector * sectors;

	if (size < sz) {
		// �N���X�^���g������
		if (false == expandClusters(sz))
			return false;
	} else {
		// �N���X�^���k������
		reduceClusters(sz);
	}

	// �f�B���N�g���G���g���Ƀt�@�C���T�C�Y�������߂�
	parent->setFileSize(entry, sz);
	fsize = sz;

	return true;
}

//=============================================================================
dword FatFile::position ()
{
	return pos;
}

//=============================================================================
dword FatFile::size ()
{
	return fsize;
}

//-----------------------------------------------------------------------------
bool FatFile::expandClusters (dword sz)
{
	dword bsize = fat->getBytesPerSector();
	dword size = bsize * sectors;
	dword count = ( pos + sz - size + bsize - 1 ) / bsize;

	// �N���X�^�̊m��
	if (count < RESIZE_DELTA)
		count = RESIZE_DELTA;

	dword cluster = fat->allocateCluster(last, count);
	if (0 == cluster)
		return false;

	if (0 < last)
		cluster = fat->getNextCluster(cluster);

	// �������m��
	dword sects = sectors + count;
	size = bsize * sects;

	byte *ptr = new byte [ size + sizeof(dword) * sects + sects ];
	if (NULL == ptr) {
		fat->freeCluster(cluster);
		if (0 < last)
			fat->setEndOfCluster(last);
		return false;
	}

	if (0 == last)
		parent->setCluster(entry, cluster);

	// �t�@�C���f�[�^���R�s�[
	memcpy(ptr, file, pos);
	memset(ptr + pos + sz, 0, size - pos - sz);

	// �Z�N�^�ʒu���R�s�[
	dword *tmplba = (dword*)( ptr + size );
	memcpy(tmplba, lba, sectors * sizeof(dword));

	// �t���O���R�s�[
	byte *tmpflag = ptr + size + sizeof(dword) * sects;
	memcpy(tmpflag, flag, sectors);

	for (dword n = sectors; n < sects; n++)
		tmpflag[n] = 1;

	// �Z�N�^�ʒu��ǂݍ���
	dword num = fat->getSectorsPerCluster();
	dword i = sectors;

	while (fat->getNumberOfClusters() > cluster) {
		dword base = fat->getLbaFromCluster(cluster);

		for (dword n = 0; n < num; n++)
			tmplba[i++] = base+n;

		last = cluster;
		cluster = fat->getNextCluster(cluster);
	}

	// �e��ݒ�l�X�V
	delete file;

	file = ptr;
	flag = tmpflag;
	lba = tmplba;
	sectors = sects;

	return true;
}

//-----------------------------------------------------------------------------
void FatFile::reduceClusters (dword sz)
{
	dword bytesPerSector = fat->getBytesPerSector();
	dword num = fat->getSectorsPerCluster();
	dword bsize = bytesPerSector * num;
	dword n = ( sz + bsize - 1 ) / bsize;

	if (sectors > n) {
		// �Z�N�^���J������ꍇ
		dword cluster = fat->getClusterFromLba(lba[n]);
		fat->freeCluster(cluster);

		if (0 == n) {
			// �f�B���N�g���G���g���ɃN���X�^�������߂�
			parent->setCluster(entry, 0);
			last = 0;
		} else {
			// ����N���X�^���m�ۂ��Ă���̂ŁA�Ō����ݒ�
			last = fat->getClusterFromLba(lba[n-1]);
			fat->setEndOfCluster(last);
		}

		sectors = n;
	}

	if (pos > sz)
		pos = sz;
}

//-----------------------------------------------------------------------------
void FatFile::clearFlag ()
{
	for (dword n = 0; n < sectors; n++) {
		flag[n] = 0;
	}
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FatDirectory
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//=============================================================================
FatDirectory::FatDirectory ()
{
	entrys = NULL;
}

//=============================================================================
FatDirectory::~FatDirectory ()
{
	delete[] entrys;
}

//=============================================================================
int FatDirectory::searchEntry (byte *bf)
{
	// �^����ꂽ���O�� 8.3 �`���ɂ���
	byte name[SIZE_FILENAME + SIZE_EXTENTION];
	expandFileName(name, bf);

	byte *tmp = entrys;

	for (int entry = 0; tmp < end; entry++, tmp += 0x20) {
		if (MARK_DELETE == tmp[0] )
			continue;
		if (ATTR_VFAT == ( tmp[ATTRIBUTE] & 0x3f ))
			continue;
		if (MARK_UNUSED == tmp[0])
			break;

		// �G���g���̖��O���r����
		bool flag = false;
		int i;

		for (i = 0; i < SIZE_FILENAME + SIZE_EXTENTION; i++) {
			byte c1 = name[i];
			byte c2 = tmp[i];

			// SJIS �̏ꍇ�̓G�X�P�[�v����
			if (false == flag) {
				if ('A' <= c1 && 'Z' >= c1) {
					c1 |= 0x20;
				} else if (0x80 & c1)
					flag = true;
				if ('A' <= c2 && 'Z' >= c2)
					c2 |= 0x20;
			} else
				flag = false;
			if (c1 != c2)
				break;
		}

		if (SIZE_FILENAME + SIZE_EXTENTION == i)
			return entry;
	}

	return -1;
}

//=============================================================================
int FatDirectory::getEntryName (int entry, byte *bf)
{
	byte *tmp = entrys + 0x20 * entry;

	if (false == isValid(tmp))
		return -1;

	byte name[SIZE_FILENAME + SIZE_EXTENTION + 2];
	int i, len = 0;

	// �K�v�Ȓ������v�Z���邾���̏ꍇ
	if (NULL == bf)
		bf = name;

	// �t�@�C�������R�s�[
	for (i = 0; i < SIZE_FILENAME; i++) {
		if (' ' == tmp[i])
			break;
		bf[len++] = tmp[i];
	}

	if (' ' != tmp[SIZE_FILENAME]) {
		bf[len++] = '.';

		// �g���q���R�s�[
		for (i = 0; i < SIZE_EXTENTION; i++) {
			if (' ' == tmp[SIZE_FILENAME + i])
				break;
			bf[len++] = tmp[SIZE_FILENAME + i];
		}
	}

	bf[len++] = '\0';

	return len;
}

//=============================================================================
int FatDirectory::getHeadEntry ()
{
	return getNextEntry(-1);
}

//=============================================================================
int FatDirectory::getNextEntry (int entry)
{
	entry++;

	if (0 > entry)
		entry = 0;

	// ���ɗL���ȃG���g����T��
	for (byte *tmp = entrys + 0x20 * entry; tmp < end; tmp += 0x20, entry++) {
		if (MARK_DELETE == tmp[0] )
			continue;
		if (ATTR_VFAT == ( tmp[ATTRIBUTE] & 0x3f ))
			continue;
		if (MARK_UNUSED == tmp[0])
			break;

		return entry;
	}

	return -1;
}

//=============================================================================
bool FatDirectory::deleteEntry (int entry)
{
	byte *tmp = entrys + 0x20 * entry;
	dword cluster = *((word*)( tmp + LOW_CLUSTER ));

	if (false == isValid(tmp))
		return false;

	if (ATTR_DIRECTORY & tmp[ATTRIBUTE]) {
		int i;

		// "." ".." �͍폜���Ȃ�
		for (i = SIZE_FILENAME + SIZE_EXTENTION - 1; 0 <= i; i--)
			if (' ' != tmp[i])
				break;

		if (2 > i) {
			for ( ; 0 <= i; i--)
				if ('.' != tmp[i])
					break;
			if (0 > i)
				return true;
		}

		if (false == clearDirectory(entry))
			return false;
	}

	dword bytesPerSector = fat->getBytesPerSector();
	dword n = ( tmp - entrys ) / bytesPerSector;

	tmp[0] = MARK_DELETE;

	// VFAT ���J�����ĉ��
	for (tmp -= 0x20; tmp >= entrys; tmp -= 0x20) {
		if (ATTR_VFAT != ( tmp[ATTRIBUTE] & 0x3f ))
			break;

		if (n != (tmp - entrys ) / bytesPerSector) {
			// �G���g�����f�B�X�N�ɏ����߂�
			fat->write(lba[n], entrys + n * bytesPerSector);
			n--;
		}

		tmp[ATTRIBUTE] = 0;

		if (tmp[0] & 0x40) {
			tmp[0] = MARK_DELETE;
			break;
		}
		tmp[0] = MARK_DELETE;
	}

	// �G���g�����f�B�X�N�ɏ����߂�
	fat->write(lba[n], entrys + n * bytesPerSector);

	// FAT �����
	fat->freeCluster(cluster);

	return true;
}

//=============================================================================
int FatDirectory::newDirectory (byte *bf)
{
	// �f�B���N�g���̖��ߐs�����p�^�[��
	dword bsize = fat->getBytesPerSector();
	dword num = fat->getSectorsPerCluster();

	byte *sector = new byte [bsize];

	// �G���g���̊m��
	dword sz = bsize * num;

	int entry = newEntry(bf, sz, ATTR_DIRECTORY, 0);
	if (-1 != entry) {
		byte *tmp = entrys + 0x20 * entry;
		dword cluster = *((word*)( tmp + LOW_CLUSTER ));

		// "." �� ".." �G���g�����쐬����
		byte name[SIZE_FILENAME + SIZE_EXTENTION];
		for (int i = 0; i < SIZE_FILENAME + SIZE_EXTENTION; i++)
			name[i] = ' ';

		memset(sector, 0, bsize);
		name[0] = '.';
		setEntry(sector, name, ATTR_DIRECTORY, cluster, 0);
		name[1] = '.';
		setEntry(sector+0x20, name, ATTR_DIRECTORY, start, 0);

		// �ŏ��̃Z�N�^����������
		dword base = fat->getLbaFromCluster(cluster);

		fat->write(base, sector);

		// �����Z�N�^����������
		memset(sector, 0, 0x20 * 2);
		for (dword n = 1; n < num; n++)
			fat->write(base+n, sector);

		// �N���X�^��H��S�ẴG���g����������
		cluster = fat->getNextCluster(cluster);

		while (cluster < fat->getNumberOfClusters()) {
			base = fat->getLbaFromCluster(cluster);
			num = fat->getSectorsPerCluster();

			for (dword n = 0; n < num; n++)
				fat->write(base+n, sector);

			cluster = fat->getNextCluster(cluster);
		}
	}

	delete sector;

	return entry;
}

//=============================================================================
int FatDirectory::newFile (byte *bf, dword sz)
{
	return newEntry(bf, sz, 0, sz);
}

//=============================================================================
Directory* FatDirectory::getDirectory (int entry)
{
	byte *tmp = entrys + 0x20 * entry;

	if (false == isValid(tmp))
		return NULL;

	if (0 == (tmp[ATTRIBUTE] & ATTR_DIRECTORY))
		return NULL;

	dword cluster = *((word*)( tmp + LOW_CLUSTER ));

	FatDirectory *dir;

	if (0 == cluster)
		dir = new FatRootDirectory();
	else
		dir = new FatSubDirectory();

	if (NULL == dir)
		return NULL;

	if (false == dir->initialize(fat, cluster)) {
		delete dir;
		return NULL;
	}

	return dir;
}

//=============================================================================
File* FatDirectory::getFile (int entry)
{
	byte *tmp = entrys + 0x20 * entry;

	if (false == isValid(tmp))
		return NULL;

	if (tmp[ATTRIBUTE] & ATTR_DIRECTORY)
		return NULL;
	if (tmp[ATTRIBUTE] & ATTR_VOLUME)
		return NULL;

	dword cluster = *((word*)( tmp + LOW_CLUSTER ));
	if (0 == cluster)
		cluster = END_OF_CLUSTER;

	dword size = *((dword*)( tmp + FILESIZE ));

	FatFile *file = new FatFile();
	if (NULL == file)
		return NULL;

	if (false == file->initialize(fat, this, entry, cluster, size)) {
		delete file;
		return NULL;
	}

	return file;
}

//=============================================================================
bool FatDirectory::isDirectory (int entry)
{
	byte *tmp = entrys + 0x20 * entry;

	if (false == isValid(tmp))
		return false;

	if (0 == (tmp[ATTRIBUTE] & ATTR_DIRECTORY))
		return false;

	return true;
}

//=============================================================================
bool FatDirectory::isFile (int entry)
{
	byte *tmp = entrys + 0x20 * entry;

	if (false == isValid(tmp))
		return false;

	if (tmp[ATTRIBUTE] & ATTR_DIRECTORY)
		return false;
	if (tmp[ATTRIBUTE] & ATTR_VOLUME)
		return false;

	return true;
}

//=============================================================================
dword FatDirectory::getIdentifer ()
{
	return start;
}

//=============================================================================
bool FatDirectory::setFileSize (int entry, dword size)
{
	byte *tmp = entrys + 0x20 * entry;

	// �t�@�C���T�C�Y
	*((dword*)( tmp + FILESIZE )) = size;

	// �G���g�����f�B�X�N�ɏ����߂�
	dword bytesPerSector = fat->getBytesPerSector();
	dword n = ( tmp - entrys ) / bytesPerSector;
	fat->write(lba[n], entrys + n * bytesPerSector);

	return true;
}

//=============================================================================
bool FatDirectory::setCluster (int entry, dword cluster)
{
	byte *tmp = entrys + 0x20 * entry;

	// �t�@�C���T�C�Y
	*((word*)( tmp + LOW_CLUSTER )) = cluster;

	// �G���g�����f�B�X�N�ɏ����߂�
	dword bytesPerSector = fat->getBytesPerSector();
	dword n = ( tmp - entrys ) / bytesPerSector;
	fat->write(lba[n], entrys + n * bytesPerSector);

	return true;
}

//-----------------------------------------------------------------------------
byte* FatDirectory::searchUnusedEntry ()
{
	byte *tmp = entrys;

	// ���g�p�G���g����T��
	while (tmp < end) {
		if (ATTR_VFAT != ( tmp[ATTRIBUTE] & 0x3f )) {
			if (MARK_UNUSED == tmp[0])
				break;
		}
		tmp += 0x20;
	}

	return tmp;
}

//-----------------------------------------------------------------------------
bool FatDirectory::isValid (byte *ent)
{
	if (entrys > ent || end <= ent)
		return false;

	if (MARK_DELETE == ent[0] )
		return false;
	if (ATTR_VFAT == ( ent[ATTRIBUTE] & 0x3f ))
		return false;
	if (MARK_UNUSED == ent[0])
		return false;
	if (' ' == ent[0])
		return false;

	return true;
}

//-----------------------------------------------------------------------------
void FatDirectory::expandFileName (byte *name, byte *bf)
{
	int i, j, index = -1;

	for (i = 0; '\0' != bf[i]; i++) {
		if ('.' == bf[i])
			index = i;
	}

	int num = i;
	if (-1 != index)
		num = index;
	if (num > SIZE_FILENAME)
		num = SIZE_FILENAME;

	for (i = 0; i < num; i++)
		name[i] = bf[i];

	while (i < SIZE_FILENAME)
		name[i++] = ' ';

	if (-1 != index) {
		for (j = index+1; '\0' != bf[j]; j++) {
			if (i >= SIZE_FILENAME + SIZE_EXTENTION)
				break;
			name[i++] = bf[j];
		}
	}

	while (i < SIZE_FILENAME + SIZE_EXTENTION)
		name[i++] = ' ';
}

//-----------------------------------------------------------------------------
bool FatDirectory::clearDirectory (int entry)
{
	bool result = true;

	Directory *dir = getDirectory(entry);
	if (NULL == dir)
		return false;

	entry = dir->getHeadEntry();
	while (-1 != entry) {
		if (false == dir->deleteEntry(entry))
			result = false;
		entry = dir->getNextEntry(entry);
	}

	delete dir;

	return result;
}

//-----------------------------------------------------------------------------
int FatDirectory::newEntry (byte *bf, dword sz, byte attr, dword fsize)
{
	// ���O�����邩�m�F����
	if ('\0' == bf[0])
		return -1;

	// �󂫃G���g����T��
	int entry = searchFreeEntry();
	if (-1 == entry)
		return -1;

	dword bsize = fat->getBytesPerSector() * fat->getSectorsPerCluster();
	dword count = ( sz + bsize-1 ) / bsize;

	// �N���X�^�̊m��
	dword cluster = 0;

	if (0 < count) {
		cluster = fat->allocateCluster(0, count);
		if (0 == cluster)
			return -1;
	}

	// �^����ꂽ���O�� 8.3 �`���ɂ���
	byte name[SIZE_FILENAME + SIZE_EXTENTION];
	expandFileName(name, bf);

	// �G���g���̍쐬
	byte *tmp = entrys + 0x20 * entry;
	setEntry(tmp, name, attr, cluster, fsize);

	// �G���g�����f�B�X�N�ɏ����߂�
	dword bytesPerSector = fat->getBytesPerSector();
	dword n = ( tmp - entrys ) / bytesPerSector;
	fat->write(lba[n], entrys + n * bytesPerSector);

	return entry;
}

//-----------------------------------------------------------------------------
void FatDirectory::setEntry (byte *ent, byte *n, byte a, word c, dword s)
{
	// ���O
	memcpy(ent, n, SIZE_FILENAME + SIZE_EXTENTION);

	// ����
	ent[ATTRIBUTE] = a;

	// �g���̈�
	ent[0x0c] = 0x00;
	ent[0x0d] = 0x00;
	ent[0x0e] = 0x00;
	ent[0x0f] = 0x00;
	ent[0x10] = 0x41;
	ent[0x11] = 0x00;
	ent[0x12] = 0x41;
	ent[0x13] = 0x00;
	ent[0x14] = 0x00;
	ent[0x15] = 0x00;
	ent[0x16] = 0x00;
	ent[0x17] = 0x00;
	ent[0x18] = 0x41;
	ent[0x19] = 0x00;

	// �J�n�N���X�^
	*((word*)( ent + LOW_CLUSTER )) = c;

	// �t�@�C���T�C�Y
	*((dword*)( ent + FILESIZE )) = s;
}

//-----------------------------------------------------------------------------
int FatDirectory::searchFreeEntry ()
{
	// ���g�p�G���g����������΍폜�G���g�����g��
	byte *tmp = entrys;
	while (tmp < unused) {
		if (MARK_DELETE == tmp[0])
			return ( tmp - entrys ) / 0x20;
		tmp += 0x20;
	}

	// ���g�p�G���g��������ΗD�悵�Ďg��
	if (tmp < end) {
		unused += 0x20;
		return ( tmp - entrys ) / 0x20;
	}

	// �Ȃ���΃G���g�����g������
	if (true == expandEntry()) {
		tmp = unused;
		unused += 0x20;
		return ( tmp - entrys ) / 0x20;
	}

	return -1;
}

//-----------------------------------------------------------------------------
bool FatDirectory::expandEntry ()
{
	if (0 == last)
		return false;

	dword cluster = fat->allocateCluster(last, RESIZE_DELTA);
	if (0 == cluster)
		return false;

	cluster = fat->getNextCluster(cluster);

	dword bsize = fat->getBytesPerSector();
	dword sects = sectors + RESIZE_DELTA;
	dword size = bsize * sects;

	byte *ptr = new byte [ size + sizeof(dword) * sects + sects ];
	if (NULL == ptr) {
		fat->freeCluster(cluster);
		fat->setEndOfCluster(last);
		return false;
	}

	// �G���g���f�[�^���R�s�[
	memcpy(ptr, entrys, bsize * sectors);
	memset(ptr + bsize * sectors, 0, bsize * (sects - sectors));

	// �Z�N�^�ʒu���R�s�[
	dword *tmplba = (dword*)( ptr + size );
	memcpy(tmplba, lba, sizeof(dword) * sectors);

	// �Z�N�^�ʒu��ǂݍ���
	dword num = fat->getSectorsPerCluster();
	dword i = sectors;

	while (fat->getNumberOfClusters() > cluster) {
		dword base = fat->getLbaFromCluster(cluster);

		for (dword n = 0; n < num; n++)
			tmplba[i++] = base+n;

		last = cluster;
		cluster = fat->getNextCluster(cluster);
	}

	delete entrys;

	// �e��p�����[�^�m��
	entrys = ptr;
	end = ptr + size;
	unused = ptr + bsize * sectors;
	lba = (dword*)( ptr + size );
	sectors = sects;

	return true;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FatRootDirectory
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//=============================================================================
bool FatRootDirectory::initialize (FAT *p, dword c)
{
	dword base = p->getRootDirectoryEntry();
	dword num = p->getNumberOfDirEntry() / ( p->getBytesPerSector() / 0x20 );
	dword sz = 0x20 * p->getNumberOfDirEntry();

	byte *ent = new byte [sz + num * sizeof(dword)];
	if (NULL == ent)
		return false;

	// Root Directory �̓ǂݍ���
	byte *tmp = ent;
	dword *tmplba = (dword*)( ent + sz );

	for (dword n = 0; n < num; n++) {
		if (false == p->read(base+n, tmp)) {
			delete[] ent;
			return false;
		}
		tmp += p->getBytesPerSector();
		tmplba[n] = base+n;
	}

	start = 0;
	last = 0;
	entrys = ent;
	end = tmp;
	lba = tmplba;
	fat = p;
	unused = searchUnusedEntry();

	return true;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FatSubDirectory
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//=============================================================================
bool FatSubDirectory::initialize (FAT *p, dword c)
{
	enum {
		DELTA = 224
	};

	// �ǂݍ���
	dword bsize = p->getBytesPerSector();
	dword sects = 0;
	byte *ptr = NULL;
	last = 0;

	if (p->getNumberOfClusters() > c) {
		ptr = p->readSectors(c, DELTA, DELTA, &sects, &last);
		if (NULL == ptr)
			return false;
	}

	// �e��p�����[�^�m��
	start = c;
	fat = p;
	entrys = ptr;
	end = ptr + bsize * sects;
	lba = (dword*)( ptr + bsize * sects );
	sectors = sects;
	unused = searchUnusedEntry();

	return true;
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
// FatStorage
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

//=============================================================================
FatStorage::FatStorage ()
{
	fat = NULL;
}

//=============================================================================
FatStorage::~FatStorage ()
{
	delete fat;
}

//=============================================================================
bool FatStorage::initialize (IStorageDevice *p)
{
	fat = new FAT;
	if (NULL == fat)
		return false;

	if (false == fat->initialize(p)) {
		delete fat;
		return false;
	}

	return true;
}

//=============================================================================
Directory* FatStorage::getRootDirectory ()
{
	FatRootDirectory *dir = new FatRootDirectory();
	if (NULL == dir)
		return NULL;

	if (false == dir->initialize(fat, 0)) {
		delete dir;
		return NULL;
	}

	return dir;
}