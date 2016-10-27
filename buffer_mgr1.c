//including required header files
#include <stdio.h>
#include <stdlib.h>
#include "buffer_mgr.h"
#include "storage_mgr.h"


typedef struct Frames
{
  char  *data;
  PageNumber pagenum;
  int dirtyPage;
  int numOfAccess;
  int freq_used;
  int nr;

}Frames;

SM_FileHandle fh;


//initializing bufferpool - creating a buffer pool for an existing page file
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData)
{
  	Frames *frameContents = malloc(numPages*sizeof(Frames));
	bm->pageFile = (char*)pageFileName;
	bm->numPages = numPages;
	bm->strategy = strategy;
	bm->bufferpage = numPages;
	bm->mgmtData = frameContents;
	int i=0;
	if(bm!=NULL){
	for(i=0;i<numPages;i++){
	frameContents[i].data = NULL;
	frameContents[i].pagenum = -1;
	frameContents[i].dirtyPage = 0;
	frameContents[i].numOfAccess = 0;
	frameContents[i].freq_used = 0;
	} 
	bm->writeio=0;
	return RC_OK;
	}
	else{
	return RC_BUFFER_INTIALISATION_FAILED;	
	}
 }


RC shutdownBufferPool(BM_BufferPool *const bm)
{
  //sd = (Structure_Details *)bm->mgmtData;
  forceFlushPool(bm);
  int i;
  for(i=0;i< bm->bufferpage;i++)
  {
    if(((Frames  *)bm->mgmtData)[i].numOfAccess != 0)
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
    if(((Frames  *)bm->mgmtData)[i].pagenum == page->pageNum)
    {
     //openPageFile (bm->pageFile, &fh);
     writeBlock (((Frames  *)bm->mgmtData)[i].pagenum, &fh, ((Frames  *)bm->mgmtData)[i].data);
      //once written dirtypage is cleared
     bm->writeio++;
     ((Frames  *)bm->mgmtData)[i].dirtyPage = 0;
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
    if((((Frames  *)bm->mgmtData)[i].dirtyPage == 1) && (((Frames  *)bm->mgmtData)[i].numOfAccess == 0)) // //checks all dirty pages (with fix count 0)
    {
      //openPageFile (bm->pageFile, &fh);
      writeBlock (((Frames  *)bm->mgmtData)[i].pagenum, &fh, ((Frames  *)bm->mgmtData)[i].data); // writing to disk from the buffer pool.
      bm->writeio++;
      ((Frames  *)bm->mgmtData)[i].dirtyPage = 0;
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
      if(((Frames  *)bm->mgmtData)[i].pagenum == page->pageNum)
      {
        ((Frames  *)bm->mgmtData)[i].dirtyPage = 1;
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
    if(((Frames  *)bm->mgmtData)[i].pagenum == page->pageNum)
    {
      ((Frames  *)bm->mgmtData)[i].numOfAccess--;
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
RC FIFO(BM_BufferPool *bm, Frames *node)
{

  int i;
  int prev = bm->next%bm -> bufferpage;
  //sd = (Structure_Details *) bm->mgmtData;
  for(i=0;i<bm -> bufferpage;i++) // checks if the page size is greater than 0
  {
    if(((Frames  *)bm->mgmtData)[prev].numOfAccess == 0)
	  {
      if(((Frames  *)bm->mgmtData)[prev].dirtyPage == 1) // checks if dirtypage is present and writes it to the disk
	    {

        writeBlock (((Frames  *)bm->mgmtData)[prev].pagenum, &fh, ((Frames  *)bm->mgmtData)[prev].data);
        bm->writeio++;
	    }
    	 ((Frames  *)bm->mgmtData)[prev].data = node->data;
    	 ((Frames  *)bm->mgmtData)[prev].pagenum = node->pagenum;
    	 ((Frames  *)bm->mgmtData)[prev].dirtyPage = node->dirtyPage;
    	 ((Frames  *)bm->mgmtData)[prev].numOfAccess = node->numOfAccess;
    	 break;
	  }

else{
       prev++;
	     if(prev%bm -> bufferpage == 0)
       prev=0;

}
  }
}

RC LRU(BM_BufferPool *const bm, Frames *node)
{
  //sd=(Structure_Details *) bm->mgmtData;
  int i;
  int prev;
  int minimum;
  for(i=0;i<bm -> bufferpage;i++)
  {
    if(((Frames  *)bm->mgmtData)[i].numOfAccess == 0)
    {
      prev= i;
      minimum = ((Frames  *)bm->mgmtData)[i].freq_used;
      break;
    }

  }
    //i=prev+1;
    for(i=prev + 1;i<bm -> bufferpage;i++)
    {
      if(((Frames  *)bm->mgmtData)[i].freq_used < minimum)
      {
        prev = i;
        minimum = ((Frames  *)bm->mgmtData)[i].freq_used;
      }

    }
    if(((Frames  *)bm->mgmtData)[prev].dirtyPage == 1) // checks if dirty page present and writes it to disk
    {

      writeBlock (((Frames  *)bm->mgmtData)[prev].pagenum, &fh, ((Frames  *)bm->mgmtData)[prev].data);
      bm->writeio++;
    }
    ((Frames  *)bm->mgmtData)[prev].data = node->data;
    ((Frames  *)bm->mgmtData)[prev].pagenum = node->pagenum;
    ((Frames  *)bm->mgmtData)[prev].dirtyPage = node->dirtyPage;
    ((Frames  *)bm->mgmtData)[prev].numOfAccess = node->numOfAccess;
    ((Frames  *)bm->mgmtData)[prev].freq_used = node->freq_used;
}
//pinPage

Frames * bufferCheck(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){

            Frames *pin = (Frames *)malloc(sizeof(Frames));
            openPageFile (bm->pageFile, &fh);
            pin->data = (SM_PageHandle) malloc(PAGE_SIZE);
            readBlock(pageNum, &fh, pin->data);
            pin->pagenum = pageNum;
            pin->dirtyPage = 0;
            pin->numOfAccess = 1;
            //pin->nr = 0;
            bm->next++;
            bm->strike++;
            pin->freq_used = bm->strike;
            page->pageNum = pageNum;
            page->data = pin->data;
            return pin;
}

//This is some enhanced at pinPage(). Can we try implementing FIFO and LRU on same function itself instead of calling to other functions 
//and also by using some switch cases to select the algorithm. 
//The LRU has some problems getting readIO. Look at it once. 
//I have seen some groups using same flow. So, I insist to change the looping to switch cases and implementing the algorithms at one function.
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	if(bm){
	BM_BufferPool *bufferPool = (BM_BufferPool *)bm->mgmtData;
	Frames *frames = (Frames *)bufferPool;
	if(bm->strategy==RS_FIFO)
	{
	if(frames[0].pagenum == -1){
	frames[0].data = (SM_PageHandle) malloc(PAGE_SIZE);
	readBlock(pageNum,&fh,frames[0].data);
	frames[0].pagenum = pageNum;
	frames[0].numOfAccess++;
	bm->next=0;
	page->pageNum = pageNum;
	page->data = frames[0].data;
	return RC_OK;
		}
	int i=0;
	int buffer_check=0;
	//if(frames[0].pagenum!=-1){
	
	for(i=0;i<bm->numPages;i++)
	{
	if(frames[0].pagenum!=-1){
		if(frames[i].pagenum==pageNum)
		{
		frames[i].numOfAccess++;
		buffer_check = 1;
		bm->strike++;
		frames[i].freq_used = bm->strike;	
		page->pageNum = pageNum;
		page->data = frames[i].data;
		break;
		}
		}
	}	if(buffer_check==0)
			{
			Frames *a =bufferCheck(bm,page,pageNum);
        		FIFO(bm,a);
			}
		return RC_OK;
	}
	int i=0;
	int buffer_check=0;
	if(bm->strategy==RS_LRU){
	for(i=0;i<bm->numPages;i++){
		if(frames[i].pagenum==pageNum){
		frames[i].numOfAccess++;
		buffer_check = 1;
		bm->strike++;
		frames[i].freq_used = bm->strike;	
		page->pageNum = pageNum;
		page->data = frames[i].data;
		break;
		}
	}if(buffer_check==0){
		Frames *a =bufferCheck(bm,page,pageNum);
        	LRU(bm,a);
				
	}return RC_OK;	
	}
	}
	
	//}
	
}

// get frame contents
PageNumber *getFrameContents (BM_BufferPool *const bm)
{
  PageNumber *val = calloc(bm -> bufferpage,sizeof(PageNumber));
  //sd= (Structure_Details *)bm->mgmtData;
  int i;
  for(i=0;i<bm -> bufferpage;i++) //function returns an array of PageNumbers (of size numPages)
  {
    val[i] = ((Frames  *)bm->mgmtData)[i].pagenum;

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
    if(((Frames  *)bm->mgmtData)[i].dirtyPage == 1)
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
  val[i] = ((Frames  *)bm->mgmtData)[i].numOfAccess;

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
