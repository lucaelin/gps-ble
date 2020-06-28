if ('wakeLock' in navigator && 'request' in navigator.wakeLock) {
  let wakeLock = null;

  const requestWakeLock = async () => {
    if (document.visibilityState !== 'visible') return;

    try {
      wakeLock = await navigator.wakeLock.request('screen');
      wakeLock.addEventListener('release', () => {
        requestWakeLock();
      });
      console.log('Wake Lock is active');
    } catch (e) {
      console.error(`${e.name}, ${e.message}`);
    }
  };

  requestWakeLock();

  const handleVisibilityChange = () => {
    requestWakeLock();
  };

  document.addEventListener('visibilitychange', handleVisibilityChange);
  document.addEventListener('fullscreenchange', handleVisibilityChange);
}
