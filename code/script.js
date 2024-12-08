async function fetchParkingData() {
  try {
    const response = await fetch('/status');  // Arduino 웹 서버의 /status 경로에서 JSON 데이터 가져오기
    if (!response.ok) throw new Error('데이터를 가져올 수 없습니다.');

    const data = await response.json();

    // 데이터 표시
    document.getElementById('current-parking').innerText = data.maxspot;
    document.getElementById('available-spots').innerText = data.carspot;
    document.getElementById('status-text').innerText = data.status; // 상태 텍스트 업데이트
  } catch (error) {
    console.error(error);
  }
}
  
  // 버튼 동작 추가
  document.getElementById('open-door-btn').onclick = function () {
    alert('문이 열렸습니다.');
  };
  
  document.getElementById('close-door-btn').onclick = function () {
    alert('문이 닫혔습니다.');
  };
  
  // 초기 데이터 로드 및 5초마다 업데이트
  window.onload = fetchParkingData;
  setInterval(fetchParkingData, 5000);
  