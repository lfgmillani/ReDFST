#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "perf.h"
#include "region.h"
#include "global.h"
#include "profile.h"
#define NUM   (c>='0'&&c<='9')

static char *gProfileFname = "redfst.profile";

static void redfst_profile_load_impl(redfst_region_t *dst){
	char buf[512];
	FILE *f;
	uint64_t nsec,ref,miss;
	int id;
	int l,r;
	int state=0;
	int line=1;
	char c;

	if(!(f = fopen(gProfileFname,"rt")))
		return;

	l = r = 0;
	while(1){
		if(l==r){
			l = 0;
			r = fread(buf,1,sizeof(buf),f);
			if(!r){
				if(feof(f)) break;
				else continue;
			}
		}
		c = buf[l++];
		switch(state){
		case 0: // _id;time;ref;miss\n
			if(NUM){
				id = c - '0';
				++state;
			}else goto FAILED;
			break;
		case 1: // ID;time;ref;miss\n
			if(NUM){
				id = 10*id + c - '0';
			}else if(';' == c){
				++state;
			}else goto FAILED;
			break;
		case 2: // _time;ref;miss\n
			if(NUM){
				nsec = c - '0';
				++state;
			}else goto FAILED;
			break;
		case 3: // TIME;ref;miss\n
			if(NUM){
				nsec = 10*nsec + c - '0';
			}else if(';' == c){
				++state;
			}else goto FAILED;
			break;
		case 4: // _ref;miss\n
			if(NUM){
				ref = c - '0';
				++state;
			}else goto FAILED;
			break;
		case 5: // REF;miss\n
			if(NUM){
				ref = 10*ref + c - '0';
			}else if(';' == c){
				++state;
			}else goto FAILED;
			break;
		case 6: // _miss\n
			if(NUM){
				miss = c - '0';
				++state;
			}else goto FAILED;
			break;
		case 7: // MISS\n
			if(NUM){
				miss = 10*miss + c - '0';
			}else if('\n' == c){
				++state;
				dst[id].perf.refs = ref;
				dst[id].perf.miss = miss;
				dst[id].time = nsec;
			}else goto FAILED;
			break;
		case 8: // \n
			++line;
			if('\n' != c){
				state = 0;
				--l;
			}
			break;
		}
	}
	if(7==state){
		dst[id].perf.refs = ref;
		dst[id].perf.miss = miss;
		dst[id].time = nsec;
	}
	fclose(f);
	return;
FAILED:
	fclose(f);
	fprintf(stderr, "failed to parse ReDFST profile at line %d\n",line);
	exit(1);
}


void redfst_profile_load(){
	int cpu,id;
	redfst_profile_load_impl(gRedfstRegion[0]);
	for(cpu=1;cpu<REDFST_MAX_THREADS;++cpu){
		for(id=0;id<REDFST_MAX_REGIONS;++id){
			gRedfstRegion[cpu][id].perf.refs = gRedfstRegion[0][id].perf.refs;
			gRedfstRegion[cpu][id].perf.miss = gRedfstRegion[0][id].perf.miss;
			gRedfstRegion[cpu][id].time = gRedfstRegion[0][id].time;
		}
	}
}

void redfst_profile_graph_save(){
	FILE *fp;
	int *hist;
	int last;
	int cpu;
	int i,j;
	hist = calloc(REDFST_MAX_REGIONS*REDFST_MAX_REGIONS, sizeof(*hist));
	last = 0;
	for(cpu=0; cpu < gRedfstThreadCount; ++cpu){
		for(i=0; i < REDFST_MAX_REGIONS; ++i){
			for(j=0; j < REDFST_MAX_REGIONS; ++j){
				if(gRedfstRegion[cpu][i].next[j]){
					if(i > last)
						last = i;
					if(j > last)
						last = j;
					hist[i*REDFST_MAX_REGIONS+j] += gRedfstRegion[cpu][i].next[j];
				}
			}
		}
	}
	fp = fopen("redfst.graph","wt");
	for(i=0;i<=last;++i){
		fprintf(fp,"%d",i);
		for(j=0;j<=last;++j)
			fprintf(fp,";%d",hist[i*REDFST_MAX_REGIONS+j]);
		fprintf(fp,"\n");
	}
	fclose(fp);
	free(hist);
}

void redfst_profile_save(){
	redfst_region_t hist[REDFST_MAX_REGIONS] = {0,};
	redfst_region_t *m;
	FILE *fp;
	uint64_t totTime,totRef,totMiss;
	int cpu;
	int region;
	redfst_profile_load_impl(hist);
	fp = fopen(gProfileFname, "wt");
	if(!fp){
		fprintf(stderr, "failed to open profile file \"%s\"\n",gProfileFname);
		return;
	}
	for(region=0;region<REDFST_MAX_REGIONS;++region){
		totTime = totRef = totMiss = 0;
		for(cpu=0;cpu<gRedfstThreadCount;++cpu){
			m = &gRedfstRegion[cpu][region];
			totTime += m->time;
			totRef += m->perf.refs;
			totMiss += m->perf.miss;
		}
		totTime -= (gRedfstThreadCount-1) * hist[region].time;
		totRef  -= (gRedfstThreadCount-1) * hist[region].perf.refs;
		totMiss -= (gRedfstThreadCount-1) * hist[region].perf.miss;
		totTime /= 2;
		totRef  /= 2;
		totMiss /= 2;
		if(totTime)
			fprintf(fp,"%d;%"PRIu64";%"PRIu64";%"PRIu64"\n",region,totTime,totRef,totMiss);
	}
	fclose(fp);
}
