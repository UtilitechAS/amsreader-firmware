import Tooltip from './Tooltip.svelte';

export function tooltip(element) {
	let title;
	let tooltipComponent;
	
    function click(event) {
        if(tooltipComponent) tooltipComponent.$destroy();

		title = element.dataset.title || element.getAttribute('title');
        var rect = element.getBoundingClientRect();

		tooltipComponent = new Tooltip({
			props: {
				title: title,
				x: rect.left + window.scrollX + (rect.width / 2),
				y: rect.top + window.scrollY,
			},
			target: document.body,
		});
	}

	function mouseLeave() {
        if(tooltipComponent) {
            setTimeout(() => {
                tooltipComponent.$destroy();
                tooltipComponent = null;
            }, 500);
        }
	}
	
	element.addEventListener('click', click);
    element.addEventListener('mouseleave', mouseLeave);
	
	return {
		destroy() {
			element.removeEventListener('click', click);
			element.removeEventListener('mouseleave', mouseLeave);
		}
	}
}