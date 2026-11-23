// Presentation only: the firmware owns pixels, menus and input timing.
export function displaySize({width, visualHeight, consoleTop, chromeHeight, controlsHeight, sideBySide, ceiling=12, padding=16}) {
  const availableHeight=visualHeight-Math.max(8,consoleTop)-chromeHeight-(sideBySide?0:controlsHeight)-8;
  const zoom=Math.max(1,Math.min(ceiling,Math.floor((width-padding)/128),Math.floor((availableHeight-padding)/64)));
  return {zoom,width:128*zoom,height:64*zoom,viewportHeight:64*zoom+padding};
}

export function bindConsoleLayout({workspace,viewport,canvas,controls,maxZoom,onFit}) {
  let pending=false;
  function fit() {
    const screen=window.visualViewport;
    const consoleRect=workspace.getBoundingClientRect(),displayRect=viewport.getBoundingClientRect(),controlRect=controls.getBoundingClientRect();
    const sideBySide=Math.abs(displayRect.top-controlRect.top)<2;
    const bodyHeight=sideBySide?Math.max(displayRect.height,controlRect.height):displayRect.height+controlRect.height;
    const size=displaySize({width:viewport.clientWidth,visualHeight:screen?.height||window.innerHeight,consoleTop:consoleRect.top-(screen?.offsetTop||0),chromeHeight:consoleRect.height-bodyHeight,controlsHeight:controlRect.height,sideBySide,ceiling:maxZoom()});
    // Keep every source pixel an integer number of CSS pixels, including when
    // browser chrome or the keyboard changes the available viewport height.
    viewport.style.height=`${size.viewportHeight}px`;
    if(canvas.width!==size.width)canvas.width=size.width;
    if(canvas.height!==size.height)canvas.height=size.height;
    onFit(size);
  }
  function schedule(){if(pending)return;pending=true;requestAnimationFrame(()=>{pending=false;fit();});}
  const observer=new ResizeObserver(schedule);
  for(const element of [workspace,viewport,controls,document.querySelector('.masthead')])if(element)observer.observe(element);
  window.addEventListener('resize',schedule);
  window.addEventListener('orientationchange',schedule);
  window.addEventListener('pageshow',schedule);
  window.visualViewport?.addEventListener('resize',schedule);
  window.visualViewport?.addEventListener('scroll',schedule);
  function revealLab(){if(location.hash==='#funkmonitor')document.getElementById('lab-tools').open=true;}
  revealLab();window.addEventListener('hashchange',revealLab);
  schedule();
  return {fit};
}
