/* $Id: cjieba.c,v 1.0 2021/11/1 sasanalyser                      $  */
/*-------------------------------------------------------------------*/
/* NAME:         CJIEBA                                              */
/* PRODUCT:      SAS/TOOLKIT                                         */
/* LANGUAGE:     C                                                   */
/* TYPE:         PROCEDURE                                           */
/* PURPOSE:      A procedure to call jieba function in SAS           */

#include "lib/jieba.h"

#include <string.h>
#include <stdio.h>

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#define MAINPROC 1
#define SASPROC  1
#include "uwproc.h"
#ifndef MVS
#include <memory.h>
#endif

char* trimwhitespace(char* str)
{
	char* end;

	// Trim leading space
	while (isspace((unsigned char)*str)) str++;

	if (*str == 0)  // All spaces?
		return str;

	// Trim trailing space
	end = str + strlen(str) - 1;
	while (end > str && isspace((unsigned char)*end)) end--;

	// Write new null terminator character
	end[1] = '\0';

	return str;
}

ptr     JIEBAG  U_PARMS((void));
void    U_MAIN(jieba)()
{
	struct XOPNSTR xopnstr;
	rctype rc;
	ptr infid, outfid;

	/*-----parse the statements-----*/
	UWPRCC(&proc);
	SAS_XSPARSE(JIEBAG(), NULL, &proc);

	int nvars, nvars2;
	int i, k, c, m, n, len;
	ptr p, xvgetptr, xvputptr, chararray, pVar;

	char* strPtr;
	char* tempVar;
	char tempName[32];
	char tmpbuf[32767];
	char newline = '\n';
	long charlens = 0;
	struct NAMESTR* storenam;
	struct NAMESTR word;

	char* p1;
	char* pvarList;

	char DICT_PATH[MAX_PATH];
	char HMM_PATH[MAX_PATH];
	char USER_DICT[MAX_PATH];
	char IDF_PATH[MAX_PATH];
	char STOP_WORDS_PATH[MAX_PATH];
	int maxWord = 32;

	if (SFTYPE(proc.head, 5) != 0) {
		maxWord = (int)SFF(proc.head, 5);
	}
	if (maxWord < 2 || maxWord > 32767) maxWord = 32;
	tempVar = (char*)SAS_XMEMEX(maxWord);

	p1 = SFLD(proc.head, 8);
	n = SAS_ZPARMN((struct STLIST*)p1);
	if (p1 == NULL || n == 0) {
		SAS_XPSLOG("ERROR: There is no DICTPATH parameter!");
		SAS_XEXIT(XEXITNORMAL, 0);
	}
	memset(DICT_PATH, 0, MAX_PATH);
	memset(HMM_PATH, 0, MAX_PATH);
	memset(USER_DICT, 0, MAX_PATH);
	memset(IDF_PATH, 0, MAX_PATH);
	memset(STOP_WORDS_PATH, 0, MAX_PATH);

	p = SAS_ZPARM((struct STLIST*)p1, 0, &len);
	if (len > (MAX_PATH - 16)) {
		SAS_XPSLOG("ERROR: The length of the parameter DICTPATH cannot exceed %d.", MAX_PATH - 16);
		SAS_XEXIT(XEXITNORMAL, 0);
	}

	strncat(DICT_PATH, p, len);
	strncat(DICT_PATH + len, "/jieba.dict.utf8", strlen("/jieba.dict.utf8"));
	strncat(HMM_PATH, p, len);
	strncat(HMM_PATH + len, "/hmm_model.utf8", strlen("/hmm_model.utf8"));
	strncat(USER_DICT, p, len);
	strncat(USER_DICT + len, "/user.dict.utf8", strlen("/user.dict.utf8"));
	strncat(IDF_PATH, p, len);
	strncat(IDF_PATH + len, "/idf.utf8", strlen("/idf.utf8"));
	strncat(STOP_WORDS_PATH, p, len);
	strncat(STOP_WORDS_PATH + len, "/stop_words.utf8", strlen("/stop_words.utf8"));

	infid = SFFILE(proc.head, 4);
	outfid = SFFILE(proc.head, 3);

	xvputptr = NULL;
	Jieba handle = NULL;
	char* s = 0;
	CJiebaWord* words;
	CJiebaWord* x;
	handle = NewJieba(DICT_PATH, HMM_PATH, USER_DICT, IDF_PATH, STOP_WORDS_PATH);
	if (outfid != NULL) {
		SAS_XVPUTI(outfid, 1, &xvputptr);
		SAS_XVNAMEI(&word);
		word.nname = "word    ";
		word.namelen = 4;
		word.ntype = 2;
		word.nlng = maxWord;
		SAS_XVPUTD(xvputptr, &word, 0, (ptr)tempVar, 0);
		SAS_XVPUTE(xvputptr);
	}

	//fetch instr
	charlens = 0;
	p1 = SFLD(proc.head, 7);
	n = SAS_ZPARMN((struct STLIST*)p1);
	if (p1 != NULL && n > 0) {
		memset(tmpbuf, 0, 32767);
		for (i = 0; i < n; i++) {
			p = SAS_ZPARM((struct STLIST*)p1, i, &len);
			charlens += len;
			strncat(tmpbuf, p, len);
			strncat(tmpbuf, &newline, 1);
		}

		words = Cut(handle, tmpbuf, charlens);
		for (x = words; x->word; x++) {
			memset(tempVar, 0, maxWord);
#ifdef __GNUC__
			snprintf(tempVar, maxWord, "%*.*s\n", x->len, x->len, x->word);
#else
			_snprintf(tempVar, maxWord, "%*.*s\n", x->len, x->len, x->word);
#endif 

			s = trimwhitespace(tempVar);
			if (strlen(s) > 0) {
				if (outfid != NULL) {
					SAS_XVPUT(xvputptr, NULL);
					SAS_XOADD(outfid, NULL);
				}
				else {
					SAS_XPSLOG("%s", tempVar);
				}
			}
		}

		FreeWords(words);
	}

	if (infid != NULL) {
		SAS_XOINFO(infid, XO_NUMVARS, (ptr)&nvars);
		SAS_XVGETI(infid, nvars, &xvgetptr);

		n = 0;
		k = 0;

		pvarList = SFLD(proc.head, 6);
		nvars2 = SAS_ZPARMN((struct STLIST*)pvarList);

		charlens = 0;
		for (i = 1; i <= nvars; i++) {
			SAS_XVNAME(infid, i, &storenam);
			if (storenam->ntype == 2) {
				if (nvars2 > 0) {
					for (m = 0; m < nvars2; m++) {
						pVar = SAS_ZPARM((struct STLIST*)pvarList, m, &len);
						memset(tempName, 0, 32);
						strncat(tempName, pVar, len);
#ifdef __GNUC__
						if (strncasecmp(storenam->nname, tempName, len) == 0) {
#else
						if (_strnicmp(storenam->nname, tempName, len) == 0) {
#endif
							k++;
							charlens += (storenam->nlng);
						}
						}
					}
				else
				{
					k++;
					charlens += (storenam->nlng);
				}
				}
			}

		chararray = SAS_XMEMEX(charlens);
		strPtr = chararray;

		for (i = 1, c = 0; i <= nvars; i++) {
			SAS_XVNAME(infid, i, &storenam);
			if (storenam->ntype == 2) {
				if (nvars2 > 0) {
					for (m = 0; m < nvars2; m++) {
						pVar = SAS_ZPARM((struct STLIST*)pvarList, m, &len);
						memset(tempName, 0, 32);
						strncat(tempName, pVar, len);
#ifdef __GNUC__
						if (strncasecmp(storenam->nname, tempName, len) == 0) {
#else
						if (_strnicmp(storenam->nname, tempName, len) == 0) {
#endif
							SAS_XVGETD(xvgetptr, i, 0, chararray + c, storenam->nlng, XV_NOFMT);
							c += storenam->nlng;
						}
						}
					}
				else
				{
					SAS_XVGETD(xvgetptr, i, 0, chararray + c, storenam->nlng, XV_NOFMT);
					c += storenam->nlng;
				}
				}
			}

		SAS_XVGETE(xvgetptr);
		while (SAS_XBYNEXT(infid) != NULL) {
			while (SAS_XBYGET(infid) != NULL) {
				SAS_XVGET(xvgetptr, NULL);
				words = Cut(handle, chararray, charlens);
				for (x = words; x->word; x++) {
					memset(tempVar, 0, maxWord);
#ifdef __GNUC__
					snprintf(tempVar, maxWord, "%*.*s\n", x->len, x->len, x->word);
#else
					_snprintf(tempVar, maxWord, "%*.*s\n", x->len, x->len, x->word);
#endif
					s = trimwhitespace(tempVar);
					if (strlen(s) > 0) {
						if (outfid != NULL) {
							SAS_XVPUT(xvputptr, NULL);
							SAS_XOADD(outfid, NULL);
						}
						else
							SAS_XPSLOG("%s", tempVar);
					}
				}

				FreeWords(words);
			}
		}
		SAS_XVGETT(xvgetptr);
		}
	FreeJieba(handle);

	SAS_XEXIT(XEXITNORMAL, 0);
		}
