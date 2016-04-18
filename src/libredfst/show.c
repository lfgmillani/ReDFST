#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include "libredfst_config.h"
#include "region.h"
#include "global.h"

void __attribute__((destructor))
redfst_show(){
  redfst_region_t *m;
  FILE *fp;
  uint64_t totTime,totRef,totMiss;
  int cpu;
  int region;
  fp = fopen("redfst.profile","wt");
  for(region=0;region<REDFSTLIB_MAX_REGIONS;++region){
    totTime = totRef = totMiss = 0;
    for(cpu=0;cpu<REDFSTLIB_MAX_THREADS;++cpu){
      m = &gRedfstRegion[cpu][region];
      totTime += m->time;
      totRef += m->perf.refs;
      totMiss += m->perf.miss;
    }
    if(totTime)
      fprintf(fp,"%d;%"PRIu64";%"PRIu64";%"PRIu64"\n",region,totTime,totRef,totMiss);
  }
  fclose(fp);
}
