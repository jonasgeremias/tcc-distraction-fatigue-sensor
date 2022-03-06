/*#include "detect.h"
#include <stdio.h>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "dl_tool.hpp"

void detect() {
    dl::tool::Latency latency;
    // initialize
#if TWO_STAGE
    HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
    HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
#else // ONE_STAGE
    HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
#endif

    // inference
    latency.start();
#if TWO_STAGE
    std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL});
    std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL}, candidates);
#else // ONE_STAGE
    std::list<dl::detect::result_t> &results = s1.infer((uint8_t *)IMAGE_ELEMENT, {IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNEL});
#endif
    latency.end();
    latency.print("Inference latency");

    // display
    int i = 0;
    for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    {
        printf("[%d] score: %f, box: [%d, %d, %d, %d]\n", i, prediction->score, prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);

#if TWO_STAGE
        printf("    left eye: (%3d, %3d), ", prediction->keypoint[0], prediction->keypoint[1]);
        printf("right eye: (%3d, %3d)\n", prediction->keypoint[6], prediction->keypoint[7]);
        printf("    nose: (%3d, %3d)\n", prediction->keypoint[4], prediction->keypoint[5]);
        printf("    mouth left: (%3d, %3d), ", prediction->keypoint[2], prediction->keypoint[3]);
        printf("mouth right: (%3d, %3d)\n\n", prediction->keypoint[8], prediction->keypoint[9]);
#endif
    }
}*/

// #include "freertos/FreeRTOS.h"
#include <stdio.h>
#include "human_face_detect_msr01.hpp"
#include "human_face_detect_mnp01.hpp"
#include "dl_tool.hpp"

#define TWO_STAGE 1 /*<! 1: detect by two-stage which is more accurate but slower(with keypoints). */
                    /*<! 0: detect by one-stage which is less accurate but faster(without keypoints). */
const char *LOG_DETECT = "LOG_DETECT";

// #include "color_detector.hpp"
using namespace dl;
using namespace std;

int frame_detect(camera_fb_t *fb, uint8_t *_jpeg_buf, size_t _jpeg_buf_len)
{
    static int16_t sample_count = 0;
    bool savedata = 1;
    char filename[32];

    // ESP_LOGE(LOG_DETECT, "_jpeg_buf = %d", (int)fb->buf);
    // ESP_LOGE(LOG_DETECT, "_jpeg_buf_len = %d", fb->len);
    // ESP_LOGE(LOG_DETECT, "width = %d", fb->width);
    // ESP_LOGE(LOG_DETECT, "height = %d", fb->height);
    // ESP_LOGE(LOG_DETECT, "format = %d", fb->format);

    // printf("buffer da imagem\n");
    // char *p = (char *)fb->buf;
    // for (int i = 0; i < 10; i++)
    // {
    //     printf("%02x", *p);
    //     p++;
    // }
    // printf("\n");
    dl::tool::Latency latency;
    HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
    HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);

    latency.start(); // inference
    std::list<dl::detect::result_t> &candidates = s1.infer((uint16_t *)fb->buf, {(int)fb->height, (int)fb->width, 3});
    std::list<dl::detect::result_t> &results = s2.infer((uint16_t *)fb->buf, {(uint16_t)fb->height, (uint16_t)fb->width, 3}, candidates);

    latency.end();
    latency.print("Inference latency");

    int i = 0;
    char text[1000] = "";
    char *p = (char *)&text;

    if (savedata)
    {   
        int create = 0;
        p += sprintf(p, "{ \"data\":[");
        for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++)
        {
            if (!create) sprintf((char *)filename, MOUNT_POINT "/i%03d.jpg", ++sample_count);
            create = 1;
            
            // sdcard_save_file_with_name((uint8_t *)_jpeg_buf, (int)_jpeg_buf_len, (char *)&filename);
            // sprintf(filename, MOUNT_POINT "/i%03d.txt", sample_count);
            // sdcard_save_file_with_name((uint8_t *)&text, (int)strlen(text), (char *)&filename);
            // ESP_LOGW(LOG_DETECT, "LOG SALVO.....%s\n", text);
           
            p += sprintf(p, "{\"s\":%.5f, -i E:\\%03d.jpg -b (%d, %d, %d, %d)", prediction->score, sample_count, prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);

            p += sprintf(p, " -k");
            p += sprintf(p, "\"(%3d, %3d),", prediction->keypoint[0], prediction->keypoint[1]);
            p += sprintf(p, "(%3d, %3d),", prediction->keypoint[6], prediction->keypoint[7]);
            p += sprintf(p, "(%3d, %3d),", prediction->keypoint[4], prediction->keypoint[5]);
            p += sprintf(p, "(%3d, %3d),", prediction->keypoint[2], prediction->keypoint[3]);
            p += sprintf(p, "(%3d, %3d)\"},", prediction->keypoint[8], prediction->keypoint[9]);

            printf("left eye: (%3d, %3d), ", prediction->keypoint[0], prediction->keypoint[1]);
            printf("right eye: (%3d, %3d)\n", prediction->keypoint[6], prediction->keypoint[7]);
            printf("nose: (%3d, %3d)\n", prediction->keypoint[4], prediction->keypoint[5]);
            printf("mouth left: (%3d, %3d), ", prediction->keypoint[2], prediction->keypoint[3]);
            printf("mouth right: (%3d, %3d)\n\n", prediction->keypoint[8], prediction->keypoint[9]);
            
            sdcard_save_file_with_name((uint8_t *)&text, (int)strlen(text), (char *)&filename);
            ESP_LOGW(LOG_DETECT, "LOG SALVO.....%s\n", text);

            i++;
            if (i > 9)
                break;
        }

        if (*(p - 1) == ',')
        {
            p -= 1;
        }
        p += sprintf(p, "]}");

        
        if (i > 0)
        {
            // sprintf((char *)filename, MOUNT_POINT "/i%03d.jpg", ++sample_count);
            sdcard_save_file_with_name((uint8_t *)_jpeg_buf, (int)_jpeg_buf_len, (char *)&filename);
            sprintf(filename, MOUNT_POINT "/i%03d.txt", sample_count);
            sdcard_save_file_with_name((uint8_t *)&text, (int)strlen(text), (char *)&filename);
            ESP_LOGW(LOG_DETECT, "LOG SALVO.....%s\n", text);
        }
    }else  {
        if (results.begin() != results.end()) {
            ESP_LOGW(LOG_DETECT, "Rosto detectado..\n");
        }
    }

    // for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    // {
    //     p += sprintf(p, "{\"sample_%d\": {\"\":,\"\":},")

    //     ESP_LOGE(LOG_DETECT, "[%d] score: %f, box: [%d, %d, %d, %d]", i, prediction->score, prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);
    // }

    // dl::tool::Latency latency;
    //     // initialize
    // #if TWO_STAGE
    //     HumanFaceDetectMSR01 s1(0.1F, 0.5F, 10, 0.2F);
    //     HumanFaceDetectMNP01 s2(0.5F, 0.3F, 5);
    // #else // ONE_STAGE
    //     HumanFaceDetectMSR01 s1(0.3F, 0.5F, 10, 0.2F);
    // #endif

    //     // inference
    //     latency.start();
    // #if TWO_STAGE
    //     std::list<dl::detect::result_t> &candidates = s1.infer((uint8_t *)fb->buf, {(int) fb->height, (int)fb->width, 3});
    //     std::list<dl::detect::result_t> &results = s2.infer((uint8_t *)fb->buf, {(int)fb->height, (int)fb->width, 3}, candidates);
    // #else // ONE_STAGE
    //     std::list<dl::detect::result_t> &results = s1.infer((uint8_t *)fb->buf, {(uint8_t)fb->height, (uint8_t)fb->width, 3});
    // #endif
    //     latency.end();
    //     latency.print("Inference latency");

    //     // display
    //     int i = 0;
    //     for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    //     {
    //         printf("[%d] score: %f, box: [%d, %d, %d, %d]\n", i, prediction->score, prediction->box[0], prediction->box[1], prediction->box[2], prediction->box[3]);

    // #if TWO_STAGE
    //         printf("    left eye: (%3d, %3d), ", prediction->keypoint[0], prediction->keypoint[1]);
    //         printf("right eye: (%3d, %3d)\n", prediction->keypoint[6], prediction->keypoint[7]);
    //         printf("    nose: (%3d, %3d)\n", prediction->keypoint[4], prediction->keypoint[5]);
    //         printf("    mouth left: (%3d, %3d), ", prediction->keypoint[2], prediction->keypoint[3]);
    //         printf("mouth right: (%3d, %3d)\n\n", prediction->keypoint[8], prediction->keypoint[9]);
    // #endif
    //     }

    return 0;
}
