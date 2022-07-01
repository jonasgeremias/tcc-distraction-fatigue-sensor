import json
import numpy
import cv2

path = "dados_brutos/"

with open('dados_tratados/dataset.json', 'r') as f:
  dataset = json.load(f)['data']



results = []
for data in dataset:
   img = data['img']
   keypoints = data['k']
   box = data['b']
   image = cv2.imread(path + img)
   image = cv2.rectangle(image, (box[0], box[1]), (box[2], box[3]), (255, 0, 0), 1)
   keypoints = numpy.reshape(keypoints, (-1, 2))

   result = {}
   result['img'] = data['img']
   result['box1'] = box[0]
   result['box2'] = box[1]
   result['box3'] = box[2]
   result['box4'] = box[3]

   count = 0
   for keypoint in keypoints:
      c = count % 6
      if (c == 0): 
         image = cv2.circle(image, tuple(keypoint), 2, (255, 0, 0), 1)
         result['eye_l_x'] = keypoint[0]
         result['eye_l_y'] = keypoint[1]
      if (c == 1): 
         image = cv2.circle(image, tuple(keypoint), 2, (0, 255, 0), 1)
         result['eye_r_x'] = keypoint[0]
         result['eye_r_y'] = keypoint[1]
      if (c == 2): 
         image = cv2.circle(image, tuple(keypoint), 2, (0, 0, 255), 1)
         result['nose_x'] = keypoint[0]
         result['nose_y'] = keypoint[1]
      if (c == 3): 
         image = cv2.circle(image, tuple(keypoint), 2, (255, 255, 0), 1)
         result['mouth_l_x'] = keypoint[0]
         result['mouth_l_y'] = keypoint[1]
      if (c == 4): 
         image = cv2.circle(image, tuple(keypoint), 2, (0, 255, 255), 1)
         result['mouth_r_x'] = keypoint[0]
         result['mouth_r_y'] = keypoint[1]
      count += 1
   cv2.imshow('tcc', image)
   while(True):
      key = cv2.waitKey(0)
      if key == ord('s') or key == ord('S'):
         result['result'] = True
         print('True')
         break
      elif key == ord('n') or key == ord('N'):
         result['result'] = False
         print('False')
         break
      
   results.append(result)

with open('classificados_separados.json', 'w') as f:
   json.dump(results, f)