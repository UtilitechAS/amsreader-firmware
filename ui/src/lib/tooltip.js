import { mount, unmount } from 'svelte';
import Tooltip from './Tooltip.svelte';

// How long the pointer must dwell on a bar before a hover tooltip appears.
// Click (and mobile tap, which synthesizes a click) shows it immediately.
const HOVER_DELAY = 500;

export function tooltip(element) {
	let title;
	let tooltipComponent;
	let hideTimer;
	let showTimer;

	function hide() {
		if(tooltipComponent) {
			unmount(tooltipComponent);
			tooltipComponent = null;
		}
	}

	function show() {
        clearTimeout(showTimer);
        clearTimeout(hideTimer);
        hide();

		title = element.dataset.title || element.getAttribute('title');
		if(!title) return;
        var rect = element.getBoundingClientRect();

		tooltipComponent = mount(Tooltip, {
			target: document.body,
			props: {
				title: title,
				x: rect.left + window.scrollX + (rect.width / 2),
				y: rect.top + window.scrollY,
			},
		});
	}

	function mouseEnter() {
        clearTimeout(showTimer);
        showTimer = setTimeout(show, HOVER_DELAY);
	}

	function mouseLeave() {
        clearTimeout(showTimer);
        if(tooltipComponent) {
            hideTimer = setTimeout(hide, 500);
        }
	}

	element.addEventListener('click', show);
	element.addEventListener('mouseenter', mouseEnter);
    element.addEventListener('mouseleave', mouseLeave);

	return {
		destroy() {
            clearTimeout(showTimer);
            clearTimeout(hideTimer);
            hide();
			element.removeEventListener('click', show);
			element.removeEventListener('mouseenter', mouseEnter);
			element.removeEventListener('mouseleave', mouseLeave);
		}
	}
}