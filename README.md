스마트 탁상시계 프로젝트
  무선 인터넷을 사용하여 정확한 시간을 출력하고 웹페이지를 통한 알람 및 시계 설정기능 제공
  
•사용 부품

  1. NODE-MCU
  2. CDS
  3. ACTIVE-BUZZER
  4. DHT11
  5. 7-SEGMENT
  6. LED(RED)
  7. BUTTON

     
• 기능

  1. 시간,날짜 출력
    시간과 날짜를 출력한다.
    날짜와 시간 중 버튼 혹은 웹을 통해 선택 가능하다.
    웹을 통해 AM/PM과 24시간을 변경 할 수 있다.
  2. NTP를 이용한 자동 시간 설정
    WiFi를 통해 시간을 국내 NTP서버로부터 받아온다.
  3. 시간대 변경
    웹을 통해 시간대를 GMT기준으로 변경 가능하다. 기본값 +9
  4. 분 단위 시간 변경
    웹을 통해 시간을 분단위로 변경 가능하다.
  5. 특정일 알람
    원하는 날짜의 원하는 시간에 알람을 울린다.
    웹으로 설정 가능하다.
    버저와 LED로 표현한다. 버튼으로 끌 수 있다.
  6. 특정 요일 반복 알람
    특정 요일의 원하는 시간에 알람을 울린다.
    웹으로 설정 가능하다.
    버저와 LED로 표현한다. 버튼으로 끌 수 있다.
  7. 자동/수동 밝기 변경
    CDS를 이용한 자동 밝기와 수동 밝기중 선택 가능하다.
    기본 설정은 수동 밝기 7단계이며 웹으로 설정 가능하다.
  8. 웹을 통한 온,습도 확인
    웹을 통해 DHT11에서 측정된 온,습도와 체감온도를 확인 가능하다.
  9. 장치의 상태 확인
    장치가 켜져 있는지 꺼져 있는지 확인이 가능하다.
    장치의 설정과 시간을 확인 가능하다.


• 회로 구성

  1. CDS
    A0
  2. BUZZER
      D6
  3. DHT11
      D5
  4. 7-SEGMENT
    CLK : D3
    DIO : D4
  5. LED
    D6
  6. BUTTON
    D1(내부 풀업)

![image](https://github.com/kjh0819/SmartDesktopClock/assets/107752598/bf375831-4066-4be7-b785-bbd173cb7ffa)

•	시스템 구성
  1.	측정 데이터
     
    1.	DHT11으로 온습도 측정을 하여 체감온도로 계산 후 온습도 값과 함께 NODE-RED로 전송합니다.
    2.	CDS를 이용 주변의 밝기를 측정합니다. 초기 계획에서는 풀업 저항을 이용하여 해상도를 조절할 계획이었으나 사용할 저항값을 선택하기 위하여 브레드보드에서 구현 할 때 저항 없이도 실내기준 한낮의 밝은 환경과 밤의 어두운 환경을 구분 할 수 있어 사용하지 않았습니다.

    	낮, 스마트폰에서 측정한 50-60lux의 환경에서는 analogRead(A0) 기준 1024,  최대 밝기로 출력 되었습니다.
    	어두운 방안, 스마트폰에서 측정한 10-12lux의 환경에서는 약 600정도의 값으로 4-5단계의 밝기가 출력 되었습니다.
    	밤, 스마트폰에서 측정한 1-2lux의 환경에서는 121또는 그 이하의 값이 측정이 되어 최저 밝기가 세그먼트에 출력 되었습니다.
  
  3.	LED동작 점등 기능
     
    1.	ActiveBuzzer와 같이 연결하여 동시에 작동하게 하였습니다. 이를 통해 알람이 울릴때 소리와 동시에 깜빡이며 장치가 켜질때 짧은 버저소리와 함께 깜빡입니다.
  
  5.	사용한 토픽
     
    1.	SmartClock/mode24
      AM,PM모드 혹은 24시간 중 선택
    2.	SmartClock/DisplayMode		  	
      날짜,시간 중 출력할 정보 선택    
    3.	SmartClock/Brightness		    	
      밝기 수동 설정    
    4.	SmartClock/Brightness/Auto		
      밝기 자동 설정 여부 선택    
    5.	SmartClock/TimeZone			      
      TimeZone 설정    
    6.	SmartClock/TimeOffset			    
      분 단위 오프셋설정    
    7.	SmartClock/Alarm/Date			    
      특정일 알람 on off    
    8.	SmartClock/Alarm/Date/time		
      특정일 알람 시간 설정    
    9.	SmartClock/Alarm/Week		    	
      특정 요일 알람 on off    
    10.	SmartClock/Alarm/Week/time		
      특정 요일 알람 시간 설정    
    11.	SmartClock/Hum			        	
      습도 정보    
    12.	SmartClock/Temp				        
      온도 정보    
    13.	SmartClock/HeatIndex		    	
      체감온도 정보    
    14.	SmartClock/Status				      
      장치의 현재 설정 전송 요청    

      이하 장치 현재 설정 전송    
    15.	SmartClock/Status/DisplayMode	
    16.	SmartClock/Status/mode24    
    17.	SmartClock/Status/alarmdate    
    18.	SmartClock/Status/alarmdate/Status    
    19.	SmartClock/Status/alarmWeek    
    20.	SmartClock/Status/alarmWeek/Status    
    21.	SmartClock/Status/TimeZone    
    22.	SmartClock/Status/TimeOffset    
    23.	SmartClock/Status/autoBrightness    
    24.	SmartClock/Status/brightness
    25.	SmartClock/Status/Date
    26.	SmartClock/Status/Time


