import json
import numpy
import cv2

path = "dados_brutos/"

with open('dados_tratados/dataset.json', 'r') as f:
  dataset = json.load(f)['data']

for data in dataset:
    img = data['img']
    keypoints = data['k']
    box = data['b']
    image = cv2.imread(path + img)
    image = cv2.rectangle(image, (box[0], box[1]), (box[2], box[3]), (255, 0, 0), 1)
      
    keypoints = numpy.reshape(keypoints, (-1, 2))
    for keypoint in keypoints:
         image = cv2.circle(image, tuple(keypoint), 2, (0, 255, 0), 1)

    cv2.imshow('tcc', image)
    while(True):
      key = cv2.waitKey(0)
      if key == ord('s') or key == ord('S'):
        data['r'] = True
        break
      elif key == ord('n') or key == ord('N'):
        data['r'] = False
        break

with open('classificados.json', 'w') as f:
    json.dump(dataset, f)