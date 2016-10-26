//including required header files
#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"


typedef struct Structure_Details
{
  char  *data;
  PageNumber pagenum;
  int dirtyPage;
  int clientcount;
  int freq_used;
  int nr;

}Structure_Details;

//declaring and initializing values, pointers
//int buffpg_size = 0;
//int next = 0;
//int writeIO = 0;
//int strike = 0;
SM_FileHandle fh;
//Structure_Details *sd;
//Structure_Details *pin;

//initializing bufferpool - creating a buffer pool for an existing page file
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData)
{
  Structure_Details *sd = malloc(numPages*sizeof(Structure_Details));
	bm->bufferpage=numPages;
	bm->strategy = strategy;
   	bm->pageFile = (char*)pageFileName;
    	bm->numPages = numPages;
	bm->mgmtData = sd;
	int i=0;
	for(i=0;i<numPages;i++){
	sd[i].data = NULL;
	sd[i].pagenum = -1;
	sd[i].dirtyPage = 0;
	sd[i].clientcount = 0;
	sd[i].freq_used = 0;
	//sd[i].nr = 0;
	} 
	bm->writeio=0;
	return RC_OK;
 }


RC shutdownBufferPool(BM_BufferPool *const bm)
{
  //sd = (Structure_Details *)bm->mgmtData;
  forceFlushPool(bm);
  int i;
  for(i=0;i< bm->bufferpage;i++)
  {
    if(((Structure_Details  *)bm->mgmtData)[i].clientcount != 0)
    return RC_PINNED_PAGES_STILL_IN_BUFFER;

  }
  //free(sd); // destorys the buffer pool
  bm->mgmtData = NULL;
  return RC_OK;
}
RC writeDirtyPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
    //BM_BufferPool *bm;
    //BM_PageHandle *page;
     int i;
     for(i=0;i<bm -> bufferpage;i++)	// checks each page and writes the current content of the page back to the page file on disk.
    {
    if(((Structure_Details  *)bm->mgmtData)[i].pagenum == page->pageNum)
    {
     //openPageFile (bm->pageFile, &fh);
     writeBlock (((Structure_Details  *)bm->mgmtData)[i].pagenum, &fh, ((Structure_Details  *)bm->mgmtData)[i].data);
      //once written dirtypage is cleared
     bm->writeio++;
     ((Structure_Details  *)bm->mgmtData)[i].dirtyPage = 0;
    }

  }

}

//writing data and flushing pool
RC forceFlushPool(BM_BufferPool *const bm)
{
  //sd = (Structure_Details *)bm->mgmtData;

  int i;
  for(i=0;i<bm -> bufferpage;i++)
  {
    if((((Structure_Details  *)bm->mgmtData)[i].dirtyPage == 1) && (((Structure_Details  *)bm->mgmtData)[i].clientcount == 0)) // //checks all dirty pages (with fix count 0)
    {
      //openPageFile (bm->pageFile, &fh);
      writeBlock (((Structure_Details  *)bm->mgmtData)[i].pagenum, &fh, ((Structure_Details  *)bm->mgmtData)[i].data); // writing to disk from the buffer pool.
      bm->writeio++;
      ((Structure_Details  *)bm->mgmtData)[i].dirtyPage = 0;
    }


  }
  return RC_OK;
}

//make page Dirty
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  //sd = (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++)
    {
      if(((Structure_Details  *)bm->mgmtData)[i].pagenum == page->pageNum)
      {
        ((Structure_Details  *)bm->mgmtData)[i].dirtyPage = 1;
        return RC_OK;   //if success it returns OK
      }
    }
  return RC_ERROR;
}

//unPinPage
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  //sd = (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++)  // checks each page and removes the clientcount
  {
    if(((Structure_Details  *)bm->mgmtData)[i].pagenum == page->pageNum)
    {
      ((Structure_Details  *)bm->mgmtData)[i].clientcount--;
      break;
    }

  }
  return RC_OK;

}



//forePage
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page)
{
  writeDirtyPage(bm,page);
  return RC_OK;
}
//FIFO
RC FIFO(BM_BufferPool *bm, Structure_Details *node)
{

  int i;
  int prev = bm->next%bm -> bufferpage;
  //sd = (Structure_Details *) bm->mgmtData;
  for(i=0;i<bm -> bufferpage;i++) // checks if the page size is greater than 0
  {
    if(((Structure_Details  *)bm->mgmtData)[prev].clientcount == 0)
	  {
      if(((Structure_Details  *)bm->mgmtData)[prev].dirtyPage == 1) // checks if dirtypage is present and writes it to the disk
	    {

        writeBlock (((Structure_Details  *)bm->mgmtData)[prev].pagenum, &fh, ((Structure_Details  *)bm->mgmtData)[prev].data);
        bm->writeio++;
	    }
    	 ((Structure_Details  *)bm->mgmtData)[prev].data = node->data;
    	 ((Structure_Details  *)bm->mgmtData)[prev].pagenum = node->pagenum;
    	 ((Structure_Details  *)bm->mgmtData)[prev].dirtyPage = node->dirtyPage;
    	 ((Structure_Details  *)bm->mgmtData)[prev].clientcount = node->clientcount;
    	 break;
	  }

else{
       prev++;
	     if(prev%bm -> bufferpage == 0)
       prev=0;

}
  }
}

RC LRU(BM_BufferPool *const bm, Structure_Details *node)
{
  //sd=(Structure_Details *) bm->mgmtData;
  int i;
  int prev;
  int minimum;
  for(i=0;i<bm -> bufferpage;i++)
  {
    if(((Structure_Details  *)bm->mgmtData)[i].clientcount == 0)
    {
      prev= i;
      minimum = ((Structure_Details  *)bm->mgmtData)[i].freq_used;
      break;
    }

  }
    //i=prev+1;
    for(i=prev + 1;i<bm -> bufferpage;i++)
    {
      if(((Structure_Details  *)bm->mgmtData)[i].freq_used < minimum)
      {
        prev = i;
        minimum = ((Structure_Details  *)bm->mgmtData)[i].freq_used;
      }

    }
    if(((Structure_Details  *)bm->mgmtData)[prev].dirtyPage == 1) // checks if dirty page present and writes it to disk
    {

      writeBlock (((Structure_Details  *)bm->mgmtData)[prev].pagenum, &fh, ((Structure_Details  *)bm->mgmtData)[prev].data);
      bm->writeio++;
    }
    ((Structure_Details  *)bm->mgmtData)[prev].data = node->data;
    ((Structure_Details  *)bm->mgmtData)[prev].pagenum = node->pagenum;
    ((Structure_Details  *)bm->mgmtData)[prev].dirtyPage = node->dirtyPage;
    ((Structure_Details  *)bm->mgmtData)[prev].clientcount = node->clientcount;
    ((Structure_Details  *)bm->mgmtData)[prev].freq_used = node->freq_used;
}
//pinPage

Structure_Details * bufferCheck(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){

            Structure_Details *pin = (Structure_Details *)malloc(sizeof(Structure_Details));
            openPageFile (bm->pageFile, &fh);
            pin->data = (SM_PageHandle) malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, pin->data);
            pin->pagenum = pageNum;
            pin->dirtyPage = 0;
            pin->clientcount = 1;
            //pin->nr = 0;
            bm->next++;
            bm->strike++;
            pin->freq_used = bm->strike;
            page->pageNum = pageNum;
            page->data = pin->data;
            return pin;
}


RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	/*if(((Structure_Details *)bm->mgmtData)[0].pagenum==-1){
	((Structure_Details  *)bm->mgmtData)[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
   	readBlock(pageNum, &fh, ((Structure_Details  *)bm->mgmtData)[0].data);
	((Structure_Details  *)bm->mgmtData)[0].clientcount++;
   	((Structure_Details  *)bm->mgmtData)[0].pagenum = pageNum;
	bm->next=0;
	page->data=((Structure_Details  *)bm->mgmtData)[0].data;
	page->pageNum=((Structure_Details  *)bm->mgmtData)[0].pagenum;
	return RC_OK;
	}*/
	
    if(bm->strategy==RS_FIFO)
    {

  	if(((Structure_Details  *)bm->mgmtData)[0].pagenum == -1)              //if there is no page in memory
  	{
   	((Structure_Details  *)bm->mgmtData)[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
   	readBlock(pageNum, &fh, ((Structure_Details  *)bm->mgmtData)[0].data);
   	((Structure_Details  *)bm->mgmtData)[0].pagenum = pageNum;
   	((Structure_Details  *)bm->mgmtData)[0].clientcount++;
   	bm->next = 0;
   	page->pageNum = pageNum;
   	page->data = ((Structure_Details  *)bm->mgmtData)[0].data;
   	return RC_OK;
  	}

      int i;
      int buffer_check = 0;
      for(i=0;i<bm -> bufferpage;i++)
      {
        if(((Structure_Details  *)bm->mgmtData)[i].pagenum != -1)
        {
	         if(((Structure_Details  *)bm->mgmtData)[i].pagenum == pageNum)
	         {
              ((Structure_Details  *)bm->mgmtData)[i].clientcount++;
            	buffer_check = 1;
            	bm->strike++;
	           ((Structure_Details  *)bm->mgmtData)[i].freq_used = bm->strike;
	            page->pageNum = pageNum;
	            page->data = ((Structure_Details  *)bm->mgmtData)[i].data;
              break;
	         }
        }

      }
        if(buffer_check == 0)
      	{

            Structure_Details *a =bufferCheck(bm,page,pageNum);
            FIFO(bm,a);
	}
      return RC_OK;
    }
    else if(bm->strategy==RS_LRU)
	{
  //sd = (Structure_Details *)bm->mgmtData;
  	if(((Structure_Details  *)bm->mgmtData)[0].pagenum == -1)
  	{
   //openPageFile (bm->pageFile, &fh);
   	((Structure_Details  *)bm->mgmtData)[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
   //ensureCapacity(pageNum,&fh);
   	readBlock(pageNum, &fh, ((Structure_Details  *)bm->mgmtData)[0].data);
   	((Structure_Details  *)bm->mgmtData)[0].pagenum = pageNum;
   	((Structure_Details  *)bm->mgmtData)[0].clientcount++;
   	bm->next = 0;
   //sd->strike = 0;
   //sd[0].freq_used = sd->strike;
   //sd[0].nr = 0;
   	page->pageNum = pageNum;
   	page->data = ((Structure_Details  *)bm->mgmtData)[0].data;
   	return RC_OK;
  	}
      int i;
      int buffer_check = 0;
      for(i=0;i<bm -> bufferpage;i++)
      {
        if(((Structure_Details  *)bm->mgmtData)[i].pagenum != -1)
        {

	         if(((Structure_Details  *)bm->mgmtData)[i].pagenum == pageNum)
	         {
              ((Structure_Details  *)bm->mgmtData)[i].clientcount++;
            	buffer_check = 1;
            	bm->strike++;
                ((Structure_Details  *)bm->mgmtData)[i].freq_used = bm->strike;
	            page->pageNum = pageNum;
	            page->data = ((Structure_Details  *)bm->mgmtData)[i].data;
              break;
	         }
        }

        else
        {

            openPageFile (bm->pageFile, &fh);
            ((Structure_Details  *)bm->mgmtData)[i].data = (SM_PageHandle) malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, ((Structure_Details  *)bm->mgmtData)[i].data);
            ((Structure_Details  *)bm->mgmtData)[i].pagenum = pageNum;
            ((Structure_Details  *)bm->mgmtData)[i].clientcount = 1;
            ((Structure_Details  *)bm->mgmtData)[i].nr = 0;
            bm->next++;
            bm->strike++;
      	     ((Structure_Details  *)bm->mgmtData)[i].freq_used = bm->strike;
        	  page->pageNum = pageNum;
        	  page->data = ((Structure_Details  *)bm->mgmtData)[i].data;
        	  buffer_check = 1;
        	  break;
        }

      }//for close
      		if(buffer_check == 0)
      		{
              	Structure_Details *a =bufferCheck(bm,page,pageNum);
            	LRU(bm,a);
		}
      		return RC_OK;
	
  }//else if close
}

// get frame contents
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
  PageNumber *val = calloc(bm -> bufferpage,sizeof(PageNumber));
  //sd= (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++) //function returns an array of PageNumbers (of size numPages)
  {
    val[i] = ((Structure_Details  *)bm->mgmtData)[i].pagenum;

  }
  return val;
}

//get dirty flags
bool *getDirtyFlags (BM_BufferPool *const bm)
{
  bool *val = calloc(bm -> bufferpage,sizeof(bool));
  //sd= (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++) // returns an array of bools (of size numPages)
  {
    if(((Structure_Details  *)bm->mgmtData)[i].dirtyPage == 1)
    {
      val[i]= 1;
    }
    else
    {
      val[i]=0;
    }

  }
  return val;
}

//get fix counts
int *getFixCounts (BM_BufferPool *const bm)
{
  int *val = calloc(bm -> bufferpage,sizeof(int));
  //sd= (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++) //returns an array of ints (of size numPages)
  {
  val[i] = ((Structure_Details  *)bm->mgmtData)[i].clientcount;

  }
  return val;
}

int getNumReadIO (BM_BufferPool *const bm)
{
  return (bm->next+1);
}

int getNumWriteIO (BM_BufferPool *const bm)
{
  return bm->writeio;
}
