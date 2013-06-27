#define NULL (void*)0

//#define OSDFLAG_FORCE_UPDATE 16
//#define MAX_UCS 1600
//#define MAX_UCSLINES 16


int audio_id = -1;
int video_id = -1;
int dvdsub_id = -2;
int sub_utf8 = 0;
int vo_osd_changed_flag = 0;

char* audio_lang = NULL;
char* dvdsub_lang = NULL;

int subcc_enabled = -1;
int sub_justify = 0;

//int vo_osd_changed = 0;

  /*
static int vo_osd_changed_status = 0;

typedef struct mp_osd_obj_s {
    struct mp_osd_obj_s* next;
    unsigned char type;
    unsigned char alignment; // 2 bits: x;y percentages, 2 bits: x;y relative to parent; 2 bits: alignment left/right/center
    unsigned short flags;
    int x,y;
    int dxs,dys;
  //   mp_osd_bbox_t bbox; // bounding box
  //   mp_osd_bbox_t old_bbox; // the renderer will save bbox here
    union {
	struct {
	    void* sub;			// value of vo_sub at last update
	    int utbl[MAX_UCS+1];	// subtitle text
	    int xtbl[MAX_UCSLINES];	// x positions
	    int lines;			// no. of lines
	} subtitle;
	struct {
	    int elems;
	} progbar;
    } params;
    int stride;

    int allocated;
    unsigned char *alpha_buffer;
    unsigned char *bitmap_buffer;
} mp_osd_obj_t;

mp_osd_obj_t* vo_osd_list = NULL;

int vo_osd_changed(int new_value)
{
    mp_osd_obj_t* obj=vo_osd_list;
    int ret = vo_osd_changed_status;
    vo_osd_changed_status = new_value;

    while(obj){
	if(obj->type==new_value) obj->flags|=OSDFLAG_FORCE_UPDATE;
	obj=obj->next;
    }

    return ret;
}
  */
