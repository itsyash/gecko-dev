/* vim: set ft=javascript ts=2 et sw=2 tw=80: */
/* Any copyright is dedicated to the Public Domain.
 http://creativecommons.org/publicdomain/zero/1.0/ */

"use strict";

// Test the fontfamily tooltip on longhand properties

const PAGE_CONTENT = [
  '<style type="text/css">',
  '  #testElement {',
  '    font-family: cursive;',
  '    color: #333;',
  '    padding-left: 70px;',
  '  }',
  '</style>',
  '<div id="testElement">test element</div>'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,font family longhand tooltip test");

  info("Creating the test document");
  content.document.body.innerHTML = PAGE_CONTENT;

  info("Opening the rule view");
  let {toolbox, inspector, view} = yield openRuleView();

  info("Selecting the test node");
  yield selectNode("#testElement", inspector);

  yield testRuleView(view, inspector.selection.nodeFront);

  info("Opening the computed view");
  let {toolbox, inspector, view} = yield openComputedView();

  yield testComputedView(view, inspector.selection.nodeFront);
});

function* testRuleView(ruleView, nodeFront) {
  info("Testing font-family tooltips in the rule view");

  let panel = ruleView.previewTooltip.panel;

  // Check that the rule view has a tooltip and that a XUL panel has been created
  ok(ruleView.previewTooltip, "Tooltip instance exists");
  ok(panel, "XUL panel exists");

  // Get the font family property inside the rule view
  let {valueSpan} = getRuleViewProperty(ruleView, "#testElement", "font-family");

  // And verify that the tooltip gets shown on this property
  yield assertHoverTooltipOn(ruleView.previewTooltip, valueSpan);

  let images = panel.getElementsByTagName("image");
  is(images.length, 1, "Tooltip contains an image");
  ok(images[0].getAttribute("src").startsWith("data:"), "Tooltip contains a data-uri image as expected");

  let dataURL = yield getFontFamilyDataURL(valueSpan.textContent, nodeFront);
  is(images[0].getAttribute("src"), dataURL, "Tooltip contains the correct data-uri image");
}

function* testComputedView(computedView, nodeFront) {
  info("Testing font-family tooltips in the computed view");

  let panel = computedView.tooltip.panel;
  let {valueSpan} = getComputedViewProperty(computedView, "font-family");

  yield assertHoverTooltipOn(computedView.tooltip, valueSpan);

  let images = panel.getElementsByTagName("image");
  is(images.length, 1, "Tooltip contains an image");
  ok(images[0].getAttribute("src").startsWith("data:"), "Tooltip contains a data-uri image as expected");

  let dataURL = yield getFontFamilyDataURL(valueSpan.textContent, nodeFront);
  is(images[0].getAttribute("src"), dataURL, "Tooltip contains the correct data-uri image");
}
