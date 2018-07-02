/*
 * file.h
 *
 *  Created on: 2017-9-20
 *      Author: czh3898
 */

#ifndef _FILE_H_
#define  _FILE_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ReadFile(char* fileName, int width, int height, char** dataBuf);
void WriteFile(char* fileName, int width, int height, char* dataBuf);
int CompareFile(char* srcFile, char* dstFile, int width, int pitch, int height);
int CompareData(char* srcBuf, char* dstBuf, int width, int pitch, int height);

#endif /*  */
